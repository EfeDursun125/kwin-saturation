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

Adjust desktop saturation and digital vibrance on KDE Plasma 6, including
Wayland sessions. KWin Saturation combines a native KWin effect with
**Digital Vibrance**, a Qt 6 control panel for live adjustments.

- Uses KWin's existing window compositing pipeline without a separate fullscreen pass.
- Works with GPUs and drivers supported by KWin's compositing backend.

## Requirements

- A distribution that provides KWin 6 development packages.
- A C++20 compiler, CMake 3.16 or newer, Qt 6, and KDE Frameworks 6 development packages.

Plasma 5 is not supported. Distribution detection alone does not guarantee
compatibility: the installed KWin version must also support the effect's API.

## Quick installation (recommended)

The installer supports **Arch Linux / CachyOS**, **Debian / Ubuntu and compatible
derivatives**, and **Fedora** using their standard package managers.
It installs build dependencies, builds a release version, and installs the
effect, GUI, command-line helper, menu entry, and icons under `/usr`.

Download and extract the repository ZIP, or clone it:

```bash
git clone https://github.com/EfeDursun125/kwin-saturation.git
cd kwin-saturation
```

Run the installer as your normal desktop user:

```bash
./autoinstall.sh
```

The installer requests `sudo` access for dependencies and system installation.
Package managers show their normal confirmation prompts. On Arch/CachyOS, it
also performs a full system upgrade to avoid partial upgrades.

**After installation, log out of KDE Plasma and log back in.** Then open
**Digital Vibrance** from the application menu or run `kwin-saturation-gui`.
Enable **Saturation Boost** in **System Settings → Desktop Effects** if needed
(the category location can vary with the Plasma version).

To preview the commands without installing anything:

```bash
./autoinstall.sh --dry-run
```

The installer requires a writable source directory and keeps build files in
`build-autoinstall/`. Run it again after source changes to rebuild and reinstall.
Immutable Fedora variants such as Kinoite require a separate installation
method and are not handled by this script.

## Manual installation

### 1. Install dependencies

Use the command for your distribution. These commands assume KDE Plasma 6 is
already installed; they do not set up a Plasma desktop.

**Arch Linux / CachyOS**

```bash
sudo pacman -Syu --needed base-devel cmake extra-cmake-modules \
    kwin qt6-base qt6-svg qt6-tools kconfig vulkan-headers
```

**Debian / Ubuntu and compatible derivatives with Plasma 6 packages**

```bash
sudo apt-get update
sudo apt-get install build-essential cmake extra-cmake-modules \
    kwin-dev qt6-base-dev qt6-svg-dev qdbus-qt6 libkf6config-dev \
    libvulkan-dev libwayland-dev libdrm-dev
```

Releases whose repositories only provide Plasma 5 cannot build this project.
The automatic installer checks the available `kwin-dev` version before
installing dependencies.

**Fedora 40+ Based**

```bash
sudo dnf install gcc-c++ make cmake extra-cmake-modules \
    kwin-devel qt6-qtbase-devel qt6-qtsvg-devel qt6-qttools kf6-kconfig-devel \
    vulkan-headers vulkan-loader-devel
```

### 2. Build and install

From the source directory:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel
sudo cmake --install build
```

**Log out and log back in** so KWin can load the installed effect.

## Usage

### Digital Vibrance control panel

Open **Digital Vibrance** from the application menu:

```bash
kwin-saturation-gui
```

Use the slider for live adjustments, select a preset, or toggle the effect.
The system tray menu also provides quick presets and an enable/disable action.

### Command line

```bash
kwin-saturation 1.25  # boost saturation to 125%
kwin-saturation 1.0   # neutral colors
kwin-saturation 0.75  # reduce saturation to 75%
kwin-saturation 0.0   # grayscale
```

The effect must be enabled in Desktop Effects. Settings are saved in `kwinrc`
and apply immediately to the running effect.

## How it works

The effect multiplies each window's `WindowPaintData` saturation value during
KWin's paint pass. It runs late in the effect chain and uses KWin's existing
compositing shader without custom GLSL, an extra texture, or an additional
fullscreen render pass. Actual performance depends on the desktop workload
and graphics driver.

Configuration is stored in `kwinrc`:

| Group | Key | Default | Purpose |
| --- | --- | --- | --- |
| `Effect-saturationeffect` | `Saturation` | `1.25` | Saturation multiplier; `1.0` is neutral |
| `Effect-saturationeffect` | `Enabled` | `true` | Apply the configured saturation value |
| `Plugins` | `saturationeffectEnabled` | Managed by KWin | Load or unload the effect |

## Troubleshooting

**CMake cannot find Vulkan headers**

Install `vulkan-headers` on Arch/CachyOS or Fedora, or `libvulkan-dev` on
Debian/Ubuntu, then rerun the installer or the manual CMake command. This
requirement comes from KWin's exported CMake dependencies.

**CMake cannot find Qt SVG or KWin**

Install the development packages listed above. Ensure that `kwin-dev` or
`kwin-devel` belongs to Plasma 6 and matches your installed KWin.

**The GUI opens, but colors do not change**

Log out and log back in after installation. Check that **Saturation Boost** is
enabled in Desktop Effects and that the GUI's effect toggle is on.

**The effect stops working after a KWin update**

Rebuild and reinstall against the updated KWin development packages, then
log out and log back in. KWin effect APIs can change between versions.

## Uninstall

Keep the build directory used for installation. Review its
`install_manifest.txt`, then remove the files listed there. For the automatic
installer:

```bash
while IFS= read -r path; do
    sudo rm -f -- "$path"
done < build-autoinstall/install_manifest.txt
```

For a manual build, use `build/install_manifest.txt` instead. If installation
created a **Digital Vibrance** desktop shortcut, remove it separately; it is
not included in the manifest. Log out and log back in after uninstalling.
Build dependencies and saved `kwinrc` preferences are kept.

## License

[GPL-3.0-or-later](LICENSE).

