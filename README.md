# WebView2 Rainmeter Plugin

A powerful Rainmeter plugin that embeds Microsoft Edge WebView2 control to display web content or local HTML files directly in your Rainmeter skins.

![Version](https://img.shields.io/badge/version-0.0.3-blue)
![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

## ✨ Features

- 🌐 **Display Web Pages** - Load any website directly in your Rainmeter skin
- 📄 **Local HTML Files** - Display custom HTML/CSS/JavaScript content
- 🪟 **Seamless Integration** - WebView window automatically parents to skin window
- 🎮 **Full Control** - Navigate, reload, go back/forward via bang commands
- 💻 **JavaScript Support** - Full JavaScript execution with event handling
- 🎨 **Customizable** - Configure size, position, and visibility
- ⚡ **Modern** - Uses Microsoft Edge WebView2 (Chromium-based)
- 🔌 **Rainmeter API Bridge** - Access Rainmeter functions from JavaScript

## 📋 Requirements

- Windows 10/11
- Rainmeter 4.5 or later
- WebView2 Runtime (usually pre-installed on Windows 10/11)
  - If not installed, download from: [WebView2 Runtime](https://developer.microsoft.com/microsoft-edge/webview2/)

## 🚀 Quick Start

### Installation

1. Download the latest release `.rmskin` package
2. Double-click to install, or
3. Manually copy `WebView2.dll` to `%APPDATA%\Rainmeter\Plugins\`

### Basic Usage

```ini
[Rainmeter]
Update=1000

[MeasureWebView]
Measure=Plugin
Plugin=WebView2
URL=https://www.google.com
Width=1000
Height=700
X=0
Y=0
```

## 📖 Documentation

### Plugin Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `URL` | String | (empty) | URL or file path to load. Supports web URLs and local file paths |
| `Width` | Integer | 800 | Width of the WebView window in pixels |
| `Height` | Integer | 600 | Height of the WebView window in pixels |
| `X` | Integer | 0 | X position relative to skin window |
| `Y` | Integer | 0 | Y position relative to skin window |
| `Visible` | Integer | 1 | Show (1) or hide (0) the WebView window |

### Bang Commands

Execute commands using `!CommandMeasure`:

```ini
; Navigate to URL
LeftMouseUpAction=[!CommandMeasure MeasureWebView "Navigate https://example.com"]

; Reload current page
LeftMouseUpAction=[!CommandMeasure MeasureWebView "Reload"]

; Navigation controls
LeftMouseUpAction=[!CommandMeasure MeasureWebView "GoBack"]
LeftMouseUpAction=[!CommandMeasure MeasureWebView "GoForward"]

; Show/Hide WebView
LeftMouseUpAction=[!CommandMeasure MeasureWebView "Show"]
LeftMouseUpAction=[!CommandMeasure MeasureWebView "Hide"]

; Execute JavaScript
LeftMouseUpAction=[!CommandMeasure MeasureWebView "ExecuteScript alert('Hello!')"]
```

## 💡 Examples

### Example 1: Mouse Drag Test

```ini
[MeasureWebView]
Measure=Plugin
Plugin=WebView2
URL=#@#mouse-drag-test.html
Width=600
Height=400
X=0
Y=0
```

### Example 2: Web Browser Skin

```ini
[MeasureWebView]
Measure=Plugin
Plugin=WebView2
URL=https://www.rainmeter.net
Width=1200
Height=800
X=0
Y=50
```

### Example 3: Local HTML Dashboard

Create `@Resources\dashboard.html`:
```html
<!DOCTYPE html>
<html>
<head>
    <style>
        body {
            font-family: 'Segoe UI', sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 40px;
        }
    </style>
</head>
<body>
    <h1>My Dashboard</h1>
    <p>Time: <span id="clock"></span></p>
    <script>
        setInterval(() => {
            document.getElementById('clock').textContent = 
                new Date().toLocaleTimeString();
        }, 1000);
    </script>
</body>
</html>
```

## 🔌 JavaScript API Bridge

The plugin automatically injects a global `rm` object into all loaded web pages, providing seamless access to Rainmeter API functions from JavaScript.

### Reading Options from Current Measure

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `rm.ReadString(option, default)` | `option` (string), `default` (string) | Promise\<string\> | Read a string option from the current measure |
| `rm.ReadInt(option, default)` | `option` (string), `default` (number) | Promise\<number\> | Read an integer option from the current measure |
| `rm.ReadDouble(option, default)` | `option` (string), `default` (number) | Promise\<number\> | Read a double/float option from the current measure |
| `rm.ReadFormula(option, default)` | `option` (string), `default` (number) | Promise\<number\> | Read and evaluate a formula option |
| `rm.ReadPath(option, default)` | `option` (string), `default` (string) | Promise\<string\> | Read a file path option from the current measure |

### Reading from Other Sections

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `rm.ReadStringFromSection(section, option, default)` | `section` (string), `option` (string), `default` (string) | Promise\<string\> | Read a string from another section/measure |
| `rm.ReadIntFromSection(section, option, default)` | `section` (string), `option` (string), `default` (number) | Promise\<number\> | Read an integer from another section/measure |
| `rm.ReadDoubleFromSection(section, option, default)` | `section` (string), `option` (string), `default` (number) | Promise\<number\> | Read a double from another section/measure |
| `rm.ReadFormulaFromSection(section, option, default)` | `section` (string), `option` (string), `default` (number) | Promise\<number\> | Read and evaluate a formula from another section |

### Utility Functions

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `rm.ReplaceVariables(text)` | `text` (string) | Promise\<string\> | Replace Rainmeter variables in text (e.g., `#CURRENTCONFIG#`) |
| `rm.PathToAbsolute(path)` | `path` (string) | Promise\<string\> | Convert relative path to absolute path |
| `rm.Execute(command)` | `command` (string) | void | Execute a Rainmeter bang command |
| `rm.Log(message, level)` | `message` (string), `level` (string) | void | Log a message to Rainmeter log. Levels: `'Notice'`, `'Warning'`, `'Error'`, `'Debug'` |

### Information Properties

| Property | Returns | Description |
|----------|---------|-------------|
| `rm.MeasureName` | Promise\<string\> | Get the name of the current measure |
| `rm.SkinName` | Promise\<string\> | Get the name of the current skin |
| `rm.SkinWindowHandle` | Promise\<string\> | Get the window handle of the skin |
| `rm.SettingsFile` | Promise\<string\> | Get the path to Rainmeter settings file |

### Usage Examples

#### Reading Options

```javascript
// Read string option
const url = await rm.ReadString('URL', 'https://default.com');

// Read integer option  
const width = await rm.ReadInt('Width', 800);

// Read double/float option
const opacity = await rm.ReadDouble('Opacity', 1.0);

// Read formula option
const calculated = await rm.ReadFormula('MyFormula', 0);

// Read path option
const filePath = await rm.ReadPath('DataFile', '');
```

#### Reading from Other Sections

```javascript
// Read string from another measure
const cpuValue = await rm.ReadStringFromSection('MeasureCPU', 'String', '0%');

// Read integer from another section
const memoryUsage = await rm.ReadIntFromSection('MeasureRAM', 'Value', 0);

// Read double from another section
const temperature = await rm.ReadDoubleFromSection('MeasureTemp', 'Value', 0.0);

// Read formula from another section
const result = await rm.ReadFormulaFromSection('MeasureCalc', 'Formula', 0.0);
```

#### Utility Functions

```javascript
// Replace Rainmeter variables
const currentPath = await rm.ReplaceVariables('#CURRENTPATH#');
const skinPath = await rm.ReplaceVariables('#@#');

// Convert relative path to absolute
const absolutePath = await rm.PathToAbsolute('#@#data.json');

// Execute Rainmeter bang commands
rm.Execute('[!SetVariable MyVar "Hello"]');
rm.Execute('[!UpdateMeter *][!Redraw]');

// Log messages to Rainmeter log
rm.Log('JavaScript initialized', 'Notice');
rm.Log('Warning: Low memory', 'Warning');
rm.Log('Error occurred', 'Error');
rm.Log('Debug info', 'Debug');
```

#### Information Properties

```javascript
// Get measure name
const measureName = await rm.MeasureName;
console.log('Measure:', measureName);

// Get skin name
const skinName = await rm.SkinName;
console.log('Skin:', skinName);

// Get skin window handle
const handle = await rm.SkinWindowHandle;

// Get settings file path
const settingsFile = await rm.SettingsFile;
```

### Complete Example

```html
<!DOCTYPE html>
<html>
<head>
    <title>Rainmeter Integration</title>
</head>
<body>
    <h1>Rainmeter API Demo</h1>
    <button onclick="updateFromRainmeter()">Get Skin Info</button>
    <div id="output"></div>
    
    <script>
        async function updateFromRainmeter() {
            try {
                // Read values from Rainmeter
                const width = await rm.ReadInt('Width', 800);
                const skinName = await rm.SkinName;
                const measureName = await rm.MeasureName;
                
                // Display results
                document.getElementById('output').innerHTML = `
                    Skin: ${skinName}<br>
                    Measure: ${measureName}<br>
                    Width: ${width}px
                `;
                
                // Log to Rainmeter
                rm.Log('Updated from JavaScript', 'Notice');
                
                // Execute Rainmeter command
                rm.Execute('[!UpdateMeter *][!Redraw]');
            } catch (error) {
                console.error('Error:', error);
                rm.Log('Error: ' + error.message, 'Error');
            }
        }
    </script>
</body>
</html>
```

### Important Notes

- ✅ All read methods return **Promises** and should be used with `await` or `.then()`
- ✅ Execute and Log methods are **fire-and-forget** (no return value)
- ✅ Property getters (MeasureName, SkinName, etc.) also return **Promises**
- ✅ The `rm` object is **automatically available** in all pages loaded by the plugin
- ✅ No additional setup or imports required


## 🔧 Building from Source

### Prerequisites

- Visual Studio 2022 (or 2019)
- Windows 10/11 SDK
- NuGet Package Manager

### Build via PowerShell

```powershell
powershell -ExecutionPolicy Bypass -Command "& { . .\Build-CPP.ps1; Dist -major 0 -minor 0 -patch 3 }"
```

This creates:
- Plugin DLLs in `dist\` folder
- Complete `.rmskin` package for distribution

## 🐛 Troubleshooting

### WebView2 doesn't appear
- Ensure WebView2 Runtime is installed
- Check Rainmeter log for error messages
- Verify skin window has appropriate dimensions

### Mouse/Keyboard events not working
- **Fixed in v0.0.3**: Added JavaScript settings (`put_IsScriptEnabled`, `put_AreDefaultScriptDialogsEnabled`, `put_IsWebMessageEnabled`)
- Ensure you're using the latest version

### File paths not loading
- Use `#@#` for @Resources folder: `URL=#@#mypage.html`
- Or use absolute paths: `URL=C:\MyFolder\page.html`

## 📝 Technical Details

- **WebView2 SDK**: Microsoft.Web.WebView2 (v1.0.2792.45)
- **Runtime**: Uses Windows Implementation Library (WIL)
- **Architecture**: Supports both x86 and x64
- **Language**: C++17
- **User Data**: Stored in `%TEMP%\RainmeterWebView2`

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📄 License

This project is licensed under the MIT License.

## 🙏 Acknowledgments

- Microsoft Edge WebView2 team for the excellent SDK
- Rainmeter community for inspiration and support

## 📧 Contact

- **Author**: nstechbytes
- **GitHub**: [WebView2 Plugin](https://github.com/nstechbytes/WebView2)

## 🔗 Related Links

- [Rainmeter Documentation](https://docs.rainmeter.net/)
- [WebView2 Documentation](https://docs.microsoft.com/microsoft-edge/webview2/)
- [Example Skins](Resources/Skins/WebView2/)

---

**Made with ❤️ for the Rainmeter community**