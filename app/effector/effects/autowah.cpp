// autowah.cpp - オートワウの実装
#include "autowah.hpp"
#include <cmath>   // exp, pow, sin, sqrt を使う

namespace
{
    // 段階値(1 ～ 5)を、範囲外なら端に寄せる
    int ClampStage(int stage)
    {
        if (stage < 1) return 1;
        if (stage > 5) return 5;
        return stage;
    }

    // 設計書 8.1 の表。添字は「段階 - 1」
    const float kSensTable[5]  = { 0.2f, 0.4f, 0.6f, 0.8f, 1.0f };
    const float kQTable[5]     = { 0.5f, 0.8f, 1.2f, 2.0f, 3.0f };
    const float kLevelTable[5] = { 0.2f, 0.4f, 0.6f, 0.8f, 1.0f };

    // ================================================================
    //  調整用の値(Step 8)。耳で聞きながら、ここだけを変えて試す
    // ================================================================

    // 包絡線の追従の速さ。ベースの低い音(41Hz は 1 周期 24ms)の波の
    // 揺れに反応しないよう、アタックは 20ms 以上にする
    const float kAttackSec  = 0.020f;  // 音が大きくなるとき
    const float kReleaseSec = 0.300f;  // 音が小さくなるとき(余韻の間ゆっくり戻る)

    // 中心周波数の範囲と、動きの滑らかさ
    const float kFcMin       = 80.0f;    // 下限 (Hz)
    const float kFcMax       = 1500.0f;  // 上限 (Hz)
    const float kFcSmoothSec = 0.025f;   // 25ms かけて動く(ウワッという動き)

    // 感度ゲイン: 包絡線に掛けて 0 ～ 1 に広げる倍率 = Base + sens × Range
    // 弱い音でもワウが動くよう、倍率を大きめにしてある
    const float kSensGainBase  = 1.0f;
    const float kSensGainRange = 8.0f;

    // Q(鋭さ)の倍率。設計書の表の値に掛ける。1.0 なら表のとおり。
    // 大きいほど「ウワッ」とした共振のクセが強くなる(その分、音は小さくなる)
    const float kQScale = 2.5f;

    // バンドパスは通る音が少なく小さくなるので、出力を持ち上げる倍率
    const float kMakeup = 1.7f;

    // ================================================================

    // 中心周波数とフィルタ係数を作り直す間隔(サンプル数)。
    // pow や sin は重いので、毎サンプルではなく 16 サンプルに 1 回だけ計算する
    // (48kHz なら 1 秒に 3000 回。ワウの動きは 25ms 単位なので十分細かい)
    const int kControlInterval = 16;

    // 音量の切り替えを滑らかにする時間
    const float kLevelSmoothSec = 0.010f;  // 10ms

    const float kPi = 3.14159265f;

    // これより小さい値は 0 とみなす(非正規化数で遅くなるのを防ぐ)
    const float kTiny = 1.0e-15f;

    // 音を安全な値にする
    //   NaN / Inf → 0、有限で範囲外 → ±1.0 に収める、範囲内 → そのまま
    float Sanitize(float x)
    {
        if (!(x - x == 0.0f))   // NaN や Inf だと、この比較が偽になる
            return 0.0f;
        if (x > 1.0f)
            return 1.0f;
        if (x < -1.0f)
            return -1.0f;
        return x;
    }

    // 極小の値を 0 にする
    float Flush(float x)
    {
        return (x > -kTiny && x < kTiny) ? 0.0f : x;
    }
}

// 覚えている状態を消す(パラメータは変えない)
void AutoWah::Reset()
{
    env_           = 0.0f;
    fc_            = kFcMin;
    low_           = 0.0f;
    band_          = 0.0f;
    level_cur_     = level_;   // 最初から目標の音量で始める
    f_             = 0.0f;
    control_count_ = 0;        // 次のサンプルで、すぐに係数を計算する
}

void AutoWah::Prepare(float sample_rate)
{
    sample_rate_ = sample_rate;

    // 時定数(秒)から、1 サンプルあたりの追従係数を求める
    env_attack_   = 1.0f - std::exp(-1.0f / (kAttackSec      * sample_rate_));
    env_release_  = 1.0f - std::exp(-1.0f / (kReleaseSec     * sample_rate_));
    // 周波数の更新は kControlInterval サンプルに 1 回なので、その間隔ぶんの係数にする
    fc_smooth_    = 1.0f - std::exp(-(float)kControlInterval / (kFcSmoothSec * sample_rate_));
    level_smooth_ = 1.0f - std::exp(-1.0f / (kLevelSmoothSec * sample_rate_));

    Reset();
}

void AutoWah::Process(float* samples, int count)
{
    const float sens_gain = kSensGainBase + sens_ * kSensGainRange;
    const float q_eff     = q_ * kQScale;        // 実際に使う Q
    const float damping   = 1.0f / q_eff;        // Q が大きいほど小さい

    // Q が大きい(帯域が狭い)ほど通る音が減って小さくなるので、
    // √Q を掛けて、Q を変えても音量が近くなるようにする
    const float gain = kMakeup * std::sqrt(q_eff);

    for (int i = 0; i < count; ++i)
    {
        const float in = Sanitize(samples[i]);   // 入力を安全な値にする

        // --- 包絡線(Step 3) --- 毎サンプル
        float x = in;
        if (x < 0.0f)
            x = -x;
        float coef = (x > env_) ? env_attack_ : env_release_;
        env_ += coef * (x - env_);

        // --- 中心周波数とフィルタ係数(Step 4・5 の前半) --- 16 サンプルに 1 回だけ
        if (control_count_ == 0)
        {
            // √ を取って、小さい音ほど持ち上げる(弱い音でもワウが動くように)
            float t = std::sqrt(env_ * sens_gain);
            if (t > 1.0f)
                t = 1.0f;
            const float target_fc = kFcMin * std::pow(kFcMax / kFcMin, t);
            fc_ += fc_smooth_ * (target_fc - fc_);

            f_ = 2.0f * std::sin(kPi * fc_ / sample_rate_);
        }
        if (++control_count_ >= kControlInterval)
            control_count_ = 0;

        // --- バンドパスフィルタ(Step 5) --- 毎サンプル
        low_  += f_ * band_;
        const float high = in - low_ - damping * band_;
        band_ += f_ * high;

        // --- 音量(Step 6): 目標 level_ に少しずつ近づける --- 毎サンプル
        level_cur_ += level_smooth_ * (level_ - level_cur_);

        // 中心でのゲインを 1 にそろえ(× damping)、持ち上げて音量を掛けて出力
        samples[i] = Sanitize(band_ * damping * gain * level_cur_);
    }

    // ブロックの終わりに、状態の後始末をする
    env_  = Flush(env_);
    low_  = Flush(low_);
    band_ = Flush(band_);

    // 万一フィルタの状態が壊れていたら(NaN や極端に大きい値)、0 に戻す
    if (!(low_ > -1.0e6f && low_ < 1.0e6f && band_ > -1.0e6f && band_ < 1.0e6f))
    {
        low_  = 0.0f;
        band_ = 0.0f;
    }
}

void AutoWah::SetSens(int stage)
{
    sens_ = kSensTable[ClampStage(stage) - 1];
}

void AutoWah::SetQ(int stage)
{
    q_ = kQTable[ClampStage(stage) - 1];
}

void AutoWah::SetLevel(int stage)
{
    level_ = kLevelTable[ClampStage(stage) - 1];
}