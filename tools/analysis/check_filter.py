"""Fit the original's low-pass to a bilinear (RBJ / TPT-SVF) 2-pole biquad.

Reports, for several Cutoff / Res knob positions, the fitted cutoff frequency and Q
against the plugin's displayed Hz, plus a linearity check (sine at fc, rising level).
Low cutoffs (< ~300 Hz) fit slightly worse because of the spectral resolution.
"""
import numpy as np
from scipy import optimize, signal

from cc import P, render, stereo, sweep_display

sr = 44100
rng = np.random.default_rng(3)
noise = (0.05 * rng.standard_normal(sr * 8)).astype(np.float32)
X = stereo(noise)
disp = {round(v, 3): float(d) for v, d, _ in sweep_display(P["Cutoff"], 1000)}


def measure(params):
    y = render(X, dict(FiltOn=1, **params))[:, 0]
    f, pxy = signal.csd(noise, y, fs=sr, nperseg=8192)
    _, pxx = signal.welch(noise, fs=sr, nperseg=8192)
    return f, np.abs(pxy / pxx)


def biquad_mag(f, fc, q):
    w0 = 2 * np.pi * fc / sr
    alpha = np.sin(w0) / (2 * q)
    c = np.cos(w0)
    b = np.array([(1 - c) / 2, 1 - c, (1 - c) / 2])
    a = np.array([1 + alpha, -2 * c, 1 - alpha])
    return np.abs(signal.freqz(b, a, worN=f, fs=sr)[1])


print("cutoff (display Hz), res -> fitted fc, Q, rms error")
for c in (0.1, 0.3, 0.5, 0.7, 0.9, 0.95):
    for res in (0.0, 0.5, 1.0):
        f, m = measure(dict(Cutoff=c, Res=res))
        sel = (f > 40) & (f < 18000)
        shown = disp[round(c, 3)]

        def err(p):
            return (20 * np.log10(biquad_mag(f[sel], p[0], p[1]) + 1e-9)
                    - 20 * np.log10(m[sel] + 1e-9))

        r = optimize.least_squares(err, [shown, 1 + 6 * res], bounds=([20, 0.3], [18000, 50]))
        print(f"{c:.2f} ({shown:5.0f} Hz) res={res:.1f}: fc={r.x[0]:7.1f}  Q={r.x[1]:.3f}"
              f"  rms err {np.sqrt(np.mean(r.fun ** 2)):.3f} dB")

print("== linearity: sine at fc, Res 1.0, rising level (gain dB, THD %)")
fc = disp[0.5]
t = np.arange(sr * 2) / sr
for amp in (0.002, 0.05, 0.1, 0.14):
    y = render(stereo(amp * np.sin(2 * np.pi * fc * t)), dict(FiltOn=1, Cutoff=0.5, Res=1.0))[sr:, 0]
    spec = np.abs(np.fft.rfft(y * np.hanning(len(y))))
    freqs = np.fft.rfftfreq(len(y), 1 / sr)
    k = lambda h: spec[np.argmin(abs(freqs - h * fc))]
    thd = 100 * np.sqrt(sum(k(h) ** 2 for h in (2, 3, 4, 5))) / k(1)
    print(f"amp {amp:5.3f}: gain {20 * np.log10(np.sqrt(2) * np.std(y) / amp):6.2f} dB, THD {thd:.3f} %")
