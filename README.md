# WebView2 Plugin for Rainmeter

<div align="center">

![WebView2 Plugin Banner](https://img.shields.io/badge/WebView2-Rainmeter_Plugin-0078D4?style=for-the-badge&logo=microsoft-edge&logoColor=white)

**Embed modern web content directly into your Rainmeter skins with full JavaScript interactivity**

[![Version](https://img.shields.io/badge/version-0.0.3-blue?style=flat-square)](../../releases)
[![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)](LICENSE)
[![Rainmeter](https://img.shields.io/badge/rainmeter-4.5+-orange?style=flat-square)](https://www.rainmeter.net/)
[![Windows](https://img.shields.io/badge/windows-10%2B-0078D6?style=flat-square&logo=windows&logoColor=white)](https://www.microsoft.com/windows)

[📥 Download](#-installation) • [📖 Documentation](#-quick-start) • [💡 Examples](#-examples) • [🤝 Contributing](#-contributing)

</div>

---

## ✨ What Can You Build?

<table>
<tr>
<td width="33%" align="center">
<img src="https://img.icons8.com/fluency/96/000000/clock.png" width="64" alt="Clock"/><br/>
<b>Animated Widgets</b><br/>
<sub>Create stunning animated clocks, weather displays, and visualizers</sub>
</td>
<td width="33%" align="center">
<img src="https://img.icons8.com/fluency/96/000000/web.png" width="64" alt="Web"/><br/>
<b>Web Dashboards</b><br/>
<sub>Embed live web content and interactive dashboards</sub>
</td>
<td width="33%" align="center">
<img src="https://img.icons8.com/fluency/96/000000/api.png" width="64" alt="API"/><br/>
<b>Smart Integrations</b><br/>
<sub>Connect to APIs and control Rainmeter with JavaScript</sub>
</td>
</tr>
</table>

---

## 🎯 Key Features

<details open>
<summary><b>🚀 Modern Web Engine</b></summary>

Powered by Microsoft Edge WebView2, supporting:
- ✅ HTML5, CSS3, JavaScript ES6+
- ✅ Modern frameworks (React, Vue, Svelte)
- ✅ WebGL, Canvas, SVG animations
- ✅ Transparent backgrounds by default

</details>

<details open>
<summary><b>🔌 Seamless JavaScript Bridge</b></summary>

Two-way communication between web and Rainmeter:
- ✅ Call Rainmeter API from JavaScript
- ✅ Execute JavaScript from Rainmeter
- ✅ Real-time data synchronization
- ✅ Custom events and callbacks

</details>

<details open>
<summary><b>⚡ Dynamic & Flexible</b></summary>

- ✅ Load local HTML or remote URLs
- ✅ Multiple WebView instances per skin
- ✅ Hot-reload without flickering
- ✅ Developer tools (F12) built-in

</details>

---

## 📋 Requirements

> **Before you begin**, make sure you have:

| Requirement | Version | Status |
|------------|---------|---------|
| **Windows** | 10 (1803+) or 11 | ![Windows](https://img.shields.io/badge/required-critical-red?style=flat-square) |
| **Rainmeter** | 4.5 or higher | ![Rainmeter](https://img.shields.io/badge/required-critical-red?style=flat-square) |
| **WebView2 Runtime** | Latest | ![WebView2](https://img.shields.io/badge/required-critical-red?style=flat-square) |

<details>
<summary>📦 <b>Don't have WebView2 Runtime?</b></summary>

<br/>

**Good news!** Windows 11 includes it by default. For Windows 10:

1. 🔗 [Download WebView2 Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/)
2. 🎯 Choose "Evergreen Standalone Installer"
3. ⚡ Run the installer (takes ~1 minute)

</details>

---

## 📥 Installation

### 🎁 Method 1: One-Click Install (Recommended)

<div align="center">

**The easiest way to get started!**

1. 📦 [Download the `.rmskin` file](../../releases/latest)
2. 🖱️ Double-click to install
3. ✨ Done! Plugin and examples are ready to use

<sub>Rainmeter will automatically install everything you need</sub>

</div>

### 🛠️ Method 2: Manual Installation

<details>
<summary>Click to expand manual installation steps</summary>

<br/>

1. **Download** the plugin DLLs from [Releases](../../releases)

2. **Choose** the right version:
   ```
   📁 x64/WebView2.dll  ← For 64-bit Rainmeter (most common)
   📁 x32/WebView2.dll  ← For 32-bit Rainmeter
   ```

3. **Copy** to your Rainmeter plugins folder:
   ```
   %AppData%\Rainmeter\Plugins\
   ```

4. **Restart** Rainmeter

</details>

---

## 🚀 Quick Start

### Your First WebView Skin

Create a new skin with this minimal configuration:

```ini
[Rainmeter]
Update=1000

[MeasureWebView]
Measure=Plugin
Plugin=WebView2
URL=file:///#@#index.html
W=800
H=600
```

Create `index.html` in your `@Resources` folder:

```html
<!DOCTYPE html>
<html>
<head>
    <style>
        body {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            font-family: 'Segoe UI', sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
        }
        h1 { font-size: 3em; animation: fadeIn 1s; }
        @keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }
    </style>
</head>
<body>
    <h1>🎉 Hello Rainmeter!</h1>
</body>
</html>
```

**That's it!** Load the skin and see your first WebView in action.

---

## ⚙️ Configuration Options

<table>
<thead>
<tr>
<th>Option</th>
<th>Description</th>
<th>Default</th>
<th>Example</th>
</tr>
</thead>
<tbody>
<tr>
<td><code>URL</code></td>
<td>🌐 HTML file or web URL<br/><sub>Supports: <code>file:///</code>, <code>http://</code>, <code>https://</code></sub></td>
<td><em>Required</em></td>
<td><code>file:///#@#index.html</code></td>
</tr>
<tr>
<td><code>W</code></td>
<td>📏 Width in pixels</td>
<td>800</td>
<td><code>1920</code></td>
</tr>
<tr>
<td><code>H</code></td>
<td>📏 Height in pixels</td>
<td>600</td>
<td><code>1080</code></td>
</tr>
<tr>
<td><code>X</code></td>
<td>↔️ Horizontal position offset</td>
<td>0</td>
<td><code>100</code></td>
</tr>
<tr>
<td><code>Y</code></td>
<td>↕️ Vertical position offset</td>
<td>0</td>
<td><code>50</code></td>
</tr>
<tr>
<td><code>Hidden</code></td>
<td>👁️ Start hidden<br/><sub>0 = visible, 1 = hidden</sub></td>
<td>0</td>
<td><code>1</code></td>
</tr>
<tr>
<td><code>Clickthrough</code></td>
<td>🖱️ Mouse interaction<br/><sub>0 = interactive, 1 = clickthrough</sub></td>
<td>0</td>
<td><code>1</code></td>
</tr>
<tr>
<td><code>DynamicVariables</code></td>
<td>🔄 Enable live updates</td>
<td>0</td>
<td><code>1</code></td>
</tr>
</tbody>
</table>

> **💡 Pro Tip:** When `DynamicVariables=1`, the WebView updates smartly:
> - **URL changes** → Navigates without recreating
> - **Size/Position changes** → Applied instantly, no flicker
> - **Visibility changes** → Instant toggle

---

## 🎮 Bang Commands

Control your WebView with Rainmeter bangs:

<table>
<tr>
<td width="50%">

**Navigation Commands**

```ini
; Go to a URL
[!CommandMeasure MeasureWebView "Navigate https://example.com"]

; Reload current page
[!CommandMeasure MeasureWebView "Reload"]

; Browser history
[!CommandMeasure MeasureWebView "GoBack"]
[!CommandMeasure MeasureWebView "GoForward"]
```

</td>
<td width="50%">

**Control Commands**

```ini
; Execute JavaScript
[!CommandMeasure MeasureWebView "ExecuteScript alert('Hi!')"]

; Developer tools
[!CommandMeasure MeasureWebView "OpenDevTools"]
```

</td>
</tr>
</table>

---

## 🔥 JavaScript Integration

### Lifecycle Hooks

Your JavaScript can respond to Rainmeter events:

```javascript
// Called once when plugin is ready
window.OnInitialize = function() {
    console.log("🚀 WebView initialized!");
    return "Ready!"; // This becomes the measure's value
};

// Called on every Rainmeter update
window.OnUpdate = function() {
    const now = new Date().toLocaleTimeString();
    return now; // Updates measure value
};
```

> ⚠️ **Note:** JavaScript execution is asynchronous, so there's a 1-update delay between JS return and Rainmeter display. This is normal!

### Call JavaScript from Rainmeter

Use section variables to call any JavaScript function:

```ini
[MeterTemperature]
Meter=String
Text=Current temp: [MeasureWebView:CallJS('getTemperature')]°C
DynamicVariables=1
```

```javascript
// In your HTML
window.getTemperature = function() {
    return 72;
};
```

---

## 🌉 RainmeterAPI Bridge

Access Rainmeter's full power from JavaScript:

### Read Skin Options

```javascript
// Read from your measure
const refreshRate = await RainmeterAPI.ReadInt('UpdateRate', 1000);
const siteName = await RainmeterAPI.ReadString('SiteName', 'Default');

// Read from other sections
const cpuUsage = await RainmeterAPI.ReadStringFromSection('MeasureCPU', 'Value', '0');
```

### Execute Bangs

```javascript
// Set variables
await RainmeterAPI.Bang('!SetVariable MyVar "Hello World"');

// Control skins
await RainmeterAPI.Bang('!ActivateConfig "MySkin" "Variant.ini"');

// Update meters
await RainmeterAPI.Bang('!UpdateMeter MeterName');
await RainmeterAPI.Bang('!Redraw');
```

### Get Skin Information

```javascript
const skinName = await RainmeterAPI.SkinName;
const measureName = await RainmeterAPI.MeasureName;

// Replace variables
const path = await RainmeterAPI.ReplaceVariables('#@#images/logo.png');

// Get variable values
const theme = await RainmeterAPI.GetVariable('CurrentTheme');
```

### Logging

```javascript
await RainmeterAPI.Log('Debug info', 'DEBUG');
await RainmeterAPI.Log('Warning message', 'WARNING');
await RainmeterAPI.Log('Error occurred', 'ERROR');
```

### Complete API Reference

<details>
<summary>📚 <b>Click to see all available methods</b></summary>

<br/>

**Reading Options**
- `ReadString(option, defaultValue)` → `Promise<string>`
- `ReadInt(option, defaultValue)` → `Promise<number>`
- `ReadDouble(option, defaultValue)` → `Promise<number>`
- `ReadFormula(option, defaultValue)` → `Promise<number>`
- `ReadPath(option, defaultValue)` → `Promise<string>`

**Reading from Sections**
- `ReadStringFromSection(section, option, defaultValue)` → `Promise<string>`
- `ReadIntFromSection(section, option, defaultValue)` → `Promise<number>`
- `ReadDoubleFromSection(section, option, defaultValue)` → `Promise<number>`
- `ReadFormulaFromSection(section, option, defaultValue)` → `Promise<number>`

**Utility Functions**
- `ReplaceVariables(text)` → `Promise<string>`
- `GetVariable(variableName)` → `Promise<string>`
- `PathToAbsolute(relativePath)` → `Promise<string>`
- `Bang(command)` → `Promise<void>`
- `Log(message, level)` → `Promise<void>`

**Properties**
- `MeasureName` → `Promise<string>`
- `SkinName` → `Promise<string>`
- `SkinWindowHandle` → `Promise<string>`
- `SettingsFile` → `Promise<string>`

</details>

---

## 💡 Examples

The plugin includes ready-to-use example skins:

<table>
<tr>
<td align="center" width="25%">
<b>🕐 Clock</b><br/>
<sub>Animated liquid clock with smooth animations</sub>
</td>
<td align="center" width="25%">
<b>📅 Calendar</b><br/>
<sub>Interactive month view calendar</sub>
</td>
<td align="center" width="25%">
<b>⚙️ Config Reader</b><br/>
<sub>Read options from measures and sections</sub>
</td>
<td align="center" width="25%">
<b>🔧 Utilities</b><br/>
<sub>Demonstrate all API functions</sub>
</td>
</tr>
</table>

**To explore examples:**
1. Install the `.rmskin` package
2. Check your Rainmeter skins folder
3. Load example skins from Rainmeter manager

---

## 🛠️ Building from Source

<details>
<summary><b>For Developers: Build Instructions</b></summary>

<br/>

### Prerequisites

- Visual Studio 2022 with C++ desktop development
- Windows SDK
- PowerShell 5.1+

### Build Steps

```bash
# Clone repository
git clone https://github.com/yourusername/WebView2.git
cd WebView2

# Open in Visual Studio
start WebView2-Plugin.sln

# Build with PowerShell
powershell -ExecutionPolicy Bypass -Command "& {. .\Build-CPP.ps1; Dist -major 0 -minor 0 -patch 3}"
```

### Build Output

```
📁 dist/
  ├── 📁 x64/
  │   └── WebView2.dll
  ├── 📁 x32/
  │   └── WebView2.dll
  ├── WebView2_v0.0.3_x64_x86_dll.zip
  └── WebView2_v0.0.3_Alpha.rmskin
```

</details>

---

## 🆘 Troubleshooting

<details>
<summary><b>❌ "WebView2 Runtime is not installed"</b></summary>

<br/>

**Solution:** Install [WebView2 Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/)

Windows 11 has it pre-installed. For Windows 10, download and run the installer.

</details>

<details>
<summary><b>❌ "Failed to create WebView2 controller"</b></summary>

<br/>

**Try these steps:**

1. ✅ Right-click skin → **Refresh skin**
2. ✅ Restart Rainmeter completely
3. ✅ Verify WebView2 Runtime is installed
4. ✅ Check Windows Event Viewer for detailed errors

</details>

<details>
<summary><b>❌ "RainmeterAPI is not defined" in JavaScript</b></summary>

<br/>

**Solution:** Wait for page to load before accessing API:

```javascript
document.addEventListener('DOMContentLoaded', () => {
    // Now you can use RainmeterAPI
    RainmeterAPI.Log('Page loaded!', 'DEBUG');
});
```

</details>

<details>
<summary><b>❌ WebView not visible</b></summary>

<br/>

**Checklist:**

- ✅ Ensure `Hidden=0` in your measure (default is 0)
- ✅ Check URL path is correct
- ✅ Verify HTML file exists
- ✅ Look for errors in Rainmeter log
- ✅ Try: `[!CommandMeasure MeasureName "OpenDevTools"]` to debug

**Transparency tip:** The WebView has transparent background by default. Use `background: transparent;` in your CSS.

</details>

---

## 📄 License

<div align="center">

**MIT License** - Free to use, modify, and distribute

See [LICENSE](LICENSE) file for full details

</div>

---

## 🤝 Contributing

We welcome contributions! Here's how:

<table>
<tr>
<td align="center" width="20%">
<b>1. Fork</b><br/>
🍴<br/>
<sub>Fork this repo</sub>
</td>
<td align="center" width="20%">
<b>2. Branch</b><br/>
🌿<br/>
<sub>Create feature branch</sub>
</td>
<td align="center" width="20%">
<b>3. Code</b><br/>
💻<br/>
<sub>Make your changes</sub>
</td>
<td align="center" width="20%">
<b>4. Commit</b><br/>
📝<br/>
<sub>Commit with clear message</sub>
</td>
<td align="center" width="20%">
<b>5. PR</b><br/>
🚀<br/>
<sub>Open Pull Request</sub>
</td>
</tr>
</table>

```bash
git checkout -b feature/AmazingFeature
git commit -m 'Add some AmazingFeature'
git push origin feature/AmazingFeature
```

---

## 🙏 Acknowledgments

<div align="center">

Built with powerful tools and inspired by the community

**[Microsoft Edge WebView2](https://developer.microsoft.com/en-us/microsoft-edge/webview2/)** • **[Rainmeter API](https://docs.rainmeter.net/developers/plugin/plugin-anatomy/)** • **Rainmeter Community**

</div>

---

<div align="center">

### 💖 Made with love for the Rainmeter community

**[⬆ Back to Top](#webview2-plugin-for-rainmeter)**

![Made with Love](https://img.shields.io/badge/Made%20with-❤️-red?style=for-the-badge)

</div>