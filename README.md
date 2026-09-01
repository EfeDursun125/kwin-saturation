<table>
  <tr>
    <td>
      <img src="./kwin-saturation.svg" width="128" height="128" alt="KWin Saturation" />
    </td>
    <td>
      
# KWin Saturation

A KWin 6 native effect that applies a global saturation/vibrance boost...
    </td>
  </tr>
</table>

Easy with GUI, Wayland friendly "vibrance" slider for KDE Plasma. It works
on any GPU/driver that supports KWin's compositing (AMD, Intel, NVIDIA,
software rendering), because it runs entirely inside KWin itself. No XRandR
colour matrix tricks needed (those don't apply on Wayland anyway).

## How it works

The effect hooks into KWin's per window paint pass and multiplies the
saturation of every window through KWin's built-in `WindowPaintData`
saturation channel, the same mechanism KWin's own effects (e.g.
`colorblindnesscorrection`) use. It requests to run last in the effect chain
so it acts as a final color grade pass.

No custom GLSL is involved, so it stays compatible with the current KWin 6
rendering pipeline instead of relying on the old `paintScreen` shader stack
trick that stopped working when KWin moved away from compositing the whole
screen into a single texture.

## Performance

Essentially free. The saturation value is fed into KWin's existing per window
compositing shader as a uniform, the same channel KWin already applies for its
own effects. There is no offscreen rendering, no extra texture, and no
additional fullscreen render pass, just one scalar multipled before each
window is drawn. The cost is negligible even on lowend or software rendered
GPUs, and disabling the effect (setting the value to `1.0`) removes the
overhead entirely.

## Requirements

- KDE Plasma 6
- A C++ 20 compiler and CMake ≥ 3.16
- Development packages: `kwin`, `extra-cmake-modules`, `qt6-base`, `kconfig`

Arch / CachyOS:

```bash
sudo pacman -S kwin extra-cmake-modules cmake qt6-base kconfig
```

Fedora 40+:

```bash
sudo dnf install \
	kwin-devel \
	extra-cmake-modules \
	cmake \
	qt6-qtbase-devel \
	kf6-kconfig-devel
```

Optional extras (may be required depending on distro packaging):

```bash
sudo dnf install \
	kf6-coreaddons-devel \
	kf6-windowssystem-devel \
	kf6-i18n-devel \
	qt6-qttools-devel
```

Debian / Ubuntu / Mint / PikaOS (names may vary):

```bash
sudo apt install cmake extra-cmake-modules qt6-base-dev kconfig-dev kwin-dev
```

## Build and install

First step: get the source (if you don't already have it)

If you've already cloned or downloaded the repository, skip this step. Otherwise, clone the repo (recommended) or download the ZIP from the green "Code" button and extract it.

Using git (recommended):

```bash
git clone https://github.com/EfeDursun125/kwin-saturation.git
cd kwin-saturation
```

If you prefer SSH and have an SSH key set up with GitHub:

```bash
git clone git@github.com:EfeDursun125/kwin-saturation.git
cd kwin-saturation
```

No git installed? Click the green "Code" button on the repository page, choose "Download ZIP", then extract the archive and open the extracted folder.

Now build and install:

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

Then restart KWin once so it picks up the new plugin
(the screen will briefly go blank):

```bash
qdbus6 org.kde.KWin /KWin org.kde.KWin.replace
```

## Enable and move

```bash
kwriteconfig6 --file kwinrc --group Plugins --key saturationeffectEnabled true
qdbus6 org.kde.KWin /KWin org.kde.KWin.reconfigure
sudo mv kwin-saturation /usr/local/bin/kwin-saturation
sudo chmod +x /usr/local/bin/kwin-saturation
```

Or toggle "Saturation Boost" in System Settings → Window Management →
Desktop Effects.

## Adjust live

```bash
./kwin-saturation 1.25  # more vibrant
./kwin-saturation 1.0   # neutral / off
./kwin-saturation 0.75  # desaturated
```

### Digital Vibrance GUI (C++ / Qt6)

The project includes a native C++ / Qt6 desktop control panel (`kwin-saturation-gui`) inspired by NVIDIA Digital Vibrance:

```bash
kwin-saturation-gui
```

- **Features**:
  - **Live Dynamic Preview**: Real-time D-Bus & KConfig communication with zero latency.
  - **Visual Indicator**: Large digital percentage display (e.g. `125%` or `1.25x`) with dynamic preset descriptions (Esports, Cinema, Vivid, Grayscale).
  - **Quick Presets Grid**: One-click switching between Default (100%), Cinema (115%), Esports (140%), Vivid (175%), Ultra (200%), and Mono (0%).
  - **Direct Effect Toggle**: Enable or disable the KWin effect on the fly.
  - **Desktop Shortcut**: Automatically adds a launcher to your **Desktop** and Application Menu (`Digital Vibrance`).




The script writes the value to `kwinrc` and calls
`org.kde.kwin.Effects.reconfigureEffect`, which reloads just this effect's
configuration immediately, no restart needed. Alternatively, you can run the
two commands it wraps yourself:

```bash
kwriteconfig6 --file kwinrc --group "Effect-saturationeffect" --key "Saturation" 1.6
qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.reconfigureEffect saturationeffect
```

The default value is `1.25`; `1.0` leaves colors untouched.

## Configuration

| Key		  | Group					| Default | Meaning						|
|--------------|--------------------------|---------|--------------------------------|
| `Saturation` | `Effect-saturationeffect`| `1.25`   | Saturation multiplier (`>1` boost, `<1` desaturate) |

## Uninstall

```bash
sudo rm /usr/lib/qt6/plugins/kwin/effects/plugins/saturationeffect.so
qdbus6 org.kde.KWin /KWin org.kde.KWin.replace
```

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
