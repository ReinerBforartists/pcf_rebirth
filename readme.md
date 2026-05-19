# PCF Rebirth

**PCF Rebirth** is a pattern controlled filter inspired by the Pattern Controlled Filter (PCF) from Propellerhead ReBirth 338.
It combines a highly flexible 16-step sequencer with sophisticated filter emulations to create evolving, rhythmic textures.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Framework](https://img.shields.io/badge/framework-JUCE%20v8-orange.svg)

## 📸 Preview

| Main Interface | Preset Browser |
| :---: | :---: |
| ![Main Interface](img/pcf_ui.jpg) | ![Presets](img/pcf_presets.jpg) |

| DAW Automation |
| :---: |
| ![Automationclip](img/automationclip.jpg) |

## ✨ Features

### 🎹 Step Sequencer
- **16-Step Grid:** Full control over pitch and gate for every step.
- **Expressive Modulation:** Integrated pitch glide, accent smoothing, and gate slew (attack/decay).
- **Dynamic Patterns:** Real-time adjustment of pattern length (from 1 to 16 steps).
- **Visual Feedback:** Real-time indication of active steps and current playback position.

### 🎛️ Powerful Filtering
- **Two Filter Algorithms:** Choose between the **SVF (State Variable Filter)** for clean, precise filtering, or the **Moog Ladder** for a warm, characterful sound inspired by classic analog synthesizer topologies.
- **Three Filter Modes:** Each algorithm offers **Lowpass (LP)**, **Bandpass (BP)**, and **Highpass (HP)** outputs — six combinations in total.
- **Dynamic Cutoff:** The cutoff frequency is modulated by the sequencer's gate and accent levels for organic, rhythmic movement.
- **High Precision:** Sample-accurate timing and smoothed parameter ramps to prevent digital clicks.

### 💾 Preset Management
- **Factory Presets:** Includes a curated collection of Acid, Techno, Ambient, Industrial, and Experimental patterns.
- **User Presets:** Save your own creations directly within the plugin. User presets are stored locally in your application data folder.

## 🛠 Technical Details
- **Framework:** Built with [JUCE Framework version 8](https://juce.com/).
- **Language:** C++
- **Architecture:** Optimized for low-latency real-time audio processing. Parameter changes are communicated to the audio thread via atomic reads and smoothed ramps, ensuring thread safety without locks.
- **License:** MIT License.

## 🚀 Installation & Usage

### Installation
1. Download the latest release for your operating system (Windows/macOS).
2. Copy the plugin file (`.vst3`, `.component`, or `.au`) into your DAW's plugin folder:
   - **Windows:** `C:\Program Files\Common Files\VST3`
   - **macOS:** `/Library/Audio/Plug-Ins/Components` or `/Library/Audio/Plug-Ins/VST3`
3. Scan for new plugins in your Digital Audio Workstation (Ableton Live, FL Studio, Logic Pro, Bitwig, etc.).

### How to use
1. **Set the Tempo:** Use the BPM display. You can manually enter a value or toggle **SYNC** to follow your DAW's transport.
2. **Program the Sequence:**
   - Click the **Step Buttons** (numbered 1–16) to turn gates on/off.
   - Adjust the **Vertical Sliders** below each button to set the pitch per step.
3. **Sculpt the Sound:** Select a filter algorithm (**SVF** or **MOOG**) and mode (**LP / BP / HP**), then use the **Freq**, **Q Amt**, and **Env Mod** knobs to shape the filter's character.
4. **Adjust the Sequence:**
   - Set the **Pattern Length** (1–16 steps) to define how many steps are played.
   - Use the **Slew** knob to control the attack and decay of the gate modulation.
   - Toggle **RUN** to start or stop the sequencer independently of the DAW transport.
5. **Manage Presets:** Use the preset browser at the bottom to navigate factory sounds or load a starting point. Customize freely and use **SAVE AS** to store your own patterns as user presets.

## 📜 License
This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

---

**Created by [Reiner Prokein](https://github.com/ReinerBforartists)**
