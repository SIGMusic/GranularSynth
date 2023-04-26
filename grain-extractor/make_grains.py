import os
import matplotlib.pyplot as plt
import numpy as np
import scipy.io

samples_folder = "./samples"
grains_folder = "./grains-new"

normalize = lambda s: np.interp(s, (s.min(), s.max()), (-1, 1))

def detect_pitch(sound, sr, min_freq=200, max_freq=1000, thresh=0.7):
    pitch = None
    dft_size = 2**int(np.log2(len(sound)))

    min_offset, max_offset = int(sr / max_freq), int(sr / min_freq)
    
    ft = np.fft.rfft(sound, n=dft_size)
    autocorr = np.fft.irfft(ft * np.conj(ft), n=dft_size)

    peak_idx = np.argmax(autocorr[min_offset:max_offset]) + min_offset
    if autocorr[peak_idx] > thresh * autocorr[0]:
        pitch = sr / peak_idx

    return pitch

def extract_grain(sound, sr, base_freq, num_periods=4):
    assert sr == 44100

    sound = normalize(sound)

    period_len = int(num_periods * sr / base_freq)
    
    wheres = np.where(np.sign(sound[1:]) != np.sign(sound[:-1]))[0]
    best_grain, best_periodicity = None, 0
    min_offset, max_offset = int(sr / (base_freq + 20)), int(sr / (base_freq - 20))

    for start_idx in wheres:

        candidate = sound[start_idx:start_idx + period_len]
        dft_size = 2**int(np.log2(len(sound)))
        ft = np.fft.rfft(candidate, n=dft_size)
        autocorr = np.fft.irfft(ft * np.conj(ft), n=dft_size)
        periodicity = max(autocorr[min_offset:max_offset]) / autocorr[0]

        if periodicity > best_periodicity:
            best_periodicity = periodicity
            best_grain = candidate
    
    return best_grain

def main():
    for filename in os.listdir(samples_folder):
        basename,ext = filename.split(".")
        if ext == "wav":
            path = os.path.join(samples_folder, filename)
            print(f"Working on {path}")

            sr, data = scipy.io.wavfile.read(path)

            data = data.T[0][int(sr):int(sr * 1.5)]

            pitch = detect_pitch(data, sr)
            print(f"Detected pitch: {pitch}")

            # grain = extract_grain(data, sr, 440.0 if "A4" in basename else 220.0)
            grain = extract_grain(data, sr, pitch)

            fig = plt.figure(figsize=(12, 4))
            plt.plot(grain)

            basename += f".{pitch}."

            fig.savefig(os.path.join(grains_folder, basename + ".png"))
            plt.close(fig)

            scipy.io.wavfile.write(os.path.join(grains_folder, basename + ".wav"),
                                   rate=sr,
                                   data=grain)

if __name__ == "__main__":
    main()
