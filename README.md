# WebView2 Plugin for Rainmeter

A powerful Rainmeter plugin that embeds Microsoft Edge WebView2 into your skins, enabling modern web content with full JavaScript interop capabilities.

![Version](https://img.shields.io/badge/version-0.0.3-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Rainmeter](https://img.shields.io/badge/rainmeter-4.5%2B-orange)

## 🌟 Features

- **Modern Web Engine**: Leverage Microsoft Edge WebView2 for rendering modern HTML5, CSS3, and JavaScript
- **JavaScript Bridge**: Seamless two-way communication between Rainmeter and web content
- **Rainmeter API Access**: Full access to Rainmeter's API from JavaScript
- **Dynamic Content**: Load local HTML files or remote URLs
- **Event Handling**: Support for custom events and callbacks
- **Multiple Skins**: Run multiple WebView2 instances simultaneously

## 📋 Requirements

- **Windows**: Windows 10 version 1803 or later (Windows 11 recommended)
- **Rainmeter**: Version 4.5 or higher
- **WebView2 Runtime**: [Download here](https://developer.microsoft.com/en-us/microsoft-edge/webview2/) (usually pre-installed on Windows 11)

## 🚀 Installation

### Method 1: RMSKIN Package (Recommended)

1. Download the latest `.rmskin` file from the [Releases](../../releases) page
2. Double-click the `.rmskin` file to install
3. Rainmeter will automatically install the plugin and example skins

### Method 2: Manual Installation

1. Download the plugin DLLs from the [Releases](../../releases) page
2. Extract the appropriate DLL for your system:
   - `x64/WebView2.dll` for 64-bit Rainmeter
   - `x32/WebView2.dll` for 32-bit Rainmeter
3. Place the DLL in your Rainmeter plugins folder:
   - `%AppData%\Rainmeter\Plugins\`

## 📖 Usage

### Basic Skin Configuration

```ini
[Rainmeter]
Update=1000

[MeasureWebView]
Measure=Plugin
Plugin=WebView2
URL=file:///#@#index.html
Width=800
Height=600
```

### Plugin Options

| Option | Description | Default | Required |
|--------|-------------|---------|----------|
| `URL` | Path to HTML file or web URL (supports `file:///`, `http://`, `https://`) | - | Yes |
| `W` | Width of the WebView in pixels | 800 | No |
| `H` | Height of the WebView in pixels | 600 | No |
| `X` | X position offset in pixels | 0 | No |
| `Y` | Y position offset in pixels | 0 | No |
| `Hidden` | Hide the WebView on load (0 = visible, 1 = hidden) | 0 | No |

**Note**: Transparent background is always enabled by default. Developer tools (F12) are always available.

### Bang Commands

Execute commands from your skin using `[!CommandMeasure MeasureName "Command"]`:

| Command | Description | Example |
|---------|-------------|---------|
| `Navigate <url>` | Navigate to a URL (web or file path) | `[!CommandMeasure MeasureWebView "Navigate https://example.com"]` |
| `Reload` | Reload the current page | `[!CommandMeasure MeasureWebView "Reload"]` |
| `GoBack` | Navigate to the previous page in history | `[!CommandMeasure MeasureWebView "GoBack"]` |
| `GoForward` | Navigate to the next page in history | `[!CommandMeasure MeasureWebView "GoForward"]` |
| `Show` | Make the WebView visible | `[!CommandMeasure MeasureWebView "Show"]` |
| `Hide` | Hide the WebView | `[!CommandMeasure MeasureWebView "Hide"]` |
| `ExecuteScript <script>` | Execute JavaScript code in the WebView | `[!CommandMeasure MeasureWebView "ExecuteScript alert('Hello')"]` |
| `SetW <width>` | Set the width of the WebView in pixels | `[!CommandMeasure MeasureWebView "SetW 1024"]` |
| `SetH <height>` | Set the height of the WebView in pixels | `[!CommandMeasure MeasureWebView "SetH 768"]` |
| `SetX <x>` | Set the X position offset in pixels | `[!CommandMeasure MeasureWebView "SetX 100"]` |
| `SetY <y>` | Set the Y position offset in pixels | `[!CommandMeasure MeasureWebView "SetY 50"]` |
| `OpenDevTools` | Open the browser developer tools (F12) | `[!CommandMeasure MeasureWebView "OpenDevTools"]` |

## 🔌 JavaScript API

### Accessing Rainmeter from JavaScript

The plugin exposes a `RainmeterAPI` object to your web content:

```javascript
// Read skin options
const value = await RainmeterAPI.ReadString('OptionName', 'DefaultValue');
const number = await RainmeterAPI.ReadInt('NumberOption', 0);
const formula = await RainmeterAPI.ReadFormula('FormulaOption', 0);

// Read from other sections
const sectionValue = await RainmeterAPI.ReadStringFromSection('SectionName', 'OptionName', 'Default');

// Execute Rainmeter bangs
await RainmeterAPI.Bang('!SetVariable MyVar "Hello"');

// Log messages
await RainmeterAPI.Log('Debug message', 'DEBUG');

// Get skin information
const measureName = await RainmeterAPI.MeasureName;
const skinName = await RainmeterAPI.SkinName;

// Replace variables
const resolved = await RainmeterAPI.ReplaceVariables('#MyVariable#');

// Get variable values
const varValue = await RainmeterAPI.GetVariable('MyVariable');

// Convert paths
const absolutePath = await RainmeterAPI.PathToAbsolute('../folder/file.txt');
```

### API Methods Reference

#### Reading Options

| Method | Parameters | Return Type | Description |
|--------|------------|-------------|-------------|
| `ReadString(option, defaultValue)` | `option`: string<br>`defaultValue`: string | `Promise<string>` | Read string option from measure |
| `ReadInt(option, defaultValue)` | `option`: string<br>`defaultValue`: number | `Promise<number>` | Read integer option from measure |
| `ReadDouble(option, defaultValue)` | `option`: string<br>`defaultValue`: number | `Promise<number>` | Read double option from measure |
| `ReadFormula(option, defaultValue)` | `option`: string<br>`defaultValue`: number | `Promise<number>` | Read and evaluate formula option |
| `ReadPath(option, defaultValue)` | `option`: string<br>`defaultValue`: string | `Promise<string>` | Read path option and convert to absolute |

#### Reading from Sections

| Method | Parameters | Return Type | Description |
|--------|------------|-------------|-------------|
| `ReadStringFromSection(section, option, defaultValue)` | `section`: string<br>`option`: string<br>`defaultValue`: string | `Promise<string>` | Read string option from specified section |
| `ReadIntFromSection(section, option, defaultValue)` | `section`: string<br>`option`: string<br>`defaultValue`: number | `Promise<number>` | Read integer option from specified section |
| `ReadDoubleFromSection(section, option, defaultValue)` | `section`: string<br>`option`: string<br>`defaultValue`: number | `Promise<number>` | Read double option from specified section |
| `ReadFormulaFromSection(section, option, defaultValue)` | `section`: string<br>`option`: string<br>`defaultValue`: number | `Promise<number>` | Read and evaluate formula from specified section |

#### Utility Functions

| Method | Parameters | Return Type | Description |
|--------|------------|-------------|-------------|
| `ReplaceVariables(text)` | `text`: string | `Promise<string>` | Replace Rainmeter variables in text (e.g., `#Variable#`, `[MeasureName]`) |
| `GetVariable(variableName)` | `variableName`: string | `Promise<string>` | Get the value of a Rainmeter variable |
| `PathToAbsolute(relativePath)` | `relativePath`: string | `Promise<string>` | Convert relative path to absolute path |
| `Bang(command)` | `command`: string | `Promise<void>` | Execute a Rainmeter bang command |
| `Log(message, level)` | `message`: string<br>`level`: 'ERROR' \| 'WARNING' \| 'NOTICE' \| 'DEBUG' | `Promise<void>` | Log a message to Rainmeter log |

#### Properties

| Property | Type | Description |
|----------|------|-------------|
| `MeasureName` | `Promise<string>` | Get the name of the current measure |
| `SkinName` | `Promise<string>` | Get the name of the current skin |
| `SkinWindowHandle` | `Promise<string>` | Get the window handle of the skin |
| `SettingsFile` | `Promise<string>` | Get the path to Rainmeter.data file |

## 📚 Examples

The plugin includes several example skins demonstrating various features:

- **Clock** - Animated liquid clock widget
- **Calendar** - Interactive calendar
- **ReadMeasureOption** - Reading options from measures
- **ReadSectionOption** - Reading options from other sections
- **BangCommand** - Executing Rainmeter bangs
- **UtilityFunction** - Using utility functions
- **InformationProperty** - Accessing skin properties

## 🛠️ Building from Source

### Prerequisites

- Visual Studio 2022 with C++ desktop development workload
- Windows SDK
- PowerShell 5.1 or later

### Build Steps

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/WebView2.git
   cd WebView2
   ```

2. Open `WebView2-Plugin.sln` in Visual Studio

3. Build using PowerShell script:
   ```powershell
   powershell -ExecutionPolicy Bypass -Command "& {. .\Build-CPP.ps1; Dist -major 0 -minor 0 -patch 3}"
   ```

This will:
- Build both x64 and x86 versions
- Create plugin DLL ZIP file
- Package the RMSKIN installer
- Output to the `dist` folder

### Build Output

- `dist/x64/WebView2.dll` - 64-bit plugin
- `dist/x32/WebView2.dll` - 32-bit plugin
- `dist/WebView2_v0.0.3_x64_x86_dll.zip` - Plugin DLLs package
- `dist/WebView2_v0.0.3_Alpha.rmskin` - Complete skin package

## 🐛 Troubleshooting

### WebView2 Runtime Not Found

**Error**: "WebView2 Runtime is not installed"

**Solution**: Install the [WebView2 Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/)

### Controller Creation Failed

**Error**: "Failed to create WebView2 controller (HRESULT: 0x80080005)"

**Solution**: 
- Refresh the skin: Right-click skin → Refresh
- Restart Rainmeter
- Ensure WebView2 Runtime is properly installed

### JavaScript API Not Available

**Error**: `RainmeterAPI is not defined`

**Solution**:
- Ensure the page has fully loaded before accessing the API
- Wait for the `DOMContentLoaded` event:
  ```javascript
  document.addEventListener('DOMContentLoaded', () => {
      // Use RainmeterAPI here
  });
  ```

### WebView Not Visible

**Problem**: WebView doesn't appear on the skin

**Solution**:
- Check that `Hidden=0` in your measure (or omit it, as 0 is the default)
- Use the `Show` bang command: `[!CommandMeasure MeasureName "Show"]`
- Verify the URL is correct and the HTML file exists
- Check Rainmeter log for errors

**Note**: The WebView always has a transparent background by default. Just use `background: transparent;` in your HTML/CSS to see through it.


## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 🙏 Acknowledgments

- Built with [Microsoft Edge WebView2](https://developer.microsoft.com/en-us/microsoft-edge/webview2/)
- Uses the [Rainmeter API](https://docs.rainmeter.net/developers/plugin/)
- Inspired by the Rainmeter community
---

**Made with ❤️ for the Rainmeter community**
