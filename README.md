# WebView2 Rainmeter Plugin

A powerful Rainmeter plugin that embeds Microsoft Edge WebView2 control to display web content or local HTML files directly in your Rainmeter skins.

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-lightgrey)
![License](https://img.shields.io/badge/license-GPL--2.0-green)

## ✨ Features

- 🌐 **Display Web Pages** - Load any website directly in your Rainmeter skin
- 📄 **Local HTML Files** - Display custom HTML/CSS/JavaScript content
- 🪟 **Seamless Integration** - WebView window automatically parents to skin window
- 🎮 **Full Control** - Navigate, reload, go back/forward via bang commands
- 💻 **JavaScript Support** - Execute JavaScript code in the WebView
- 🎨 **Customizable** - Configure size, position, and visibility
- ⚡ **Modern** - Uses Microsoft Edge WebView2 (Chromium-based)

## 📋 Requirements

- Windows 10/11
- Rainmeter 4.5 or later
- WebView2 Runtime (usually pre-installed on Windows 10/11)
  - If not installed, download from: [WebView2 Runtime](https://developer.microsoft.com/microsoft-edge/webview2/)

## 🚀 Quick Start

### Installation

1. Download the latest release from [Releases](https://github.com/nstechbytes/WebView2/releases)
2. Install the `.rmskin` package, or
3. Manually copy `WebView2.dll` to `%APPDATA%\Rainmeter\Plugins\`

### Basic Usage

```ini
[Rainmeter]
Update=1000

[MeasureWebView]
Measure=Plugin
Plugin=WebView2
Url=https://www.google.com
Width=1000
Height=700
X=0
Y=0
Visible=1
```

## 📖 Documentation

### Plugin Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `Url` | String | (empty) | URL or file path to load. Supports web URLs and local file paths |
| `Width` | Integer | 800 | Width of the WebView window in pixels |
| `Height` | Integer | 600 | Height of the WebView window in pixels |
| `X` | Integer | 0 | X position relative to skin window |
| `Y` | Integer | 0 | Y position relative to skin window |
| `Visible` | Integer | 1 | Show (1) or hide (0) the WebView window |

### Bang Commands

Execute commands using `!CommandMeasure`:

#### Navigate to URL
```ini
LeftMouseUpAction=[!CommandMeasure MeasureWebView "Navigate https://example.com"]
```

#### Reload Current Page
```ini
LeftMouseUpAction=[!CommandMeasure MeasureWebView "Reload"]
```

#### Navigation Controls
```ini
LeftMouseUpAction=[!CommandMeasure MeasureWebView "GoBack"]
LeftMouseUpAction=[!CommandMeasure MeasureWebView "GoForward"]
```

#### Show/Hide WebView
```ini
LeftMouseUpAction=[!CommandMeasure MeasureWebView "Show"]
LeftMouseUpAction=[!CommandMeasure MeasureWebView "Hide"]
```

#### Execute JavaScript
```ini
LeftMouseUpAction=[!CommandMeasure MeasureWebView "ExecuteScript alert('Hello from Rainmeter!')"]
```

## 💡 Examples

### Example 1: Web Browser Skin

```ini
[Rainmeter]
Update=1000
BackgroundMode=2
SolidColor=30,30,30,255

[Variables]
WebWidth=1200
WebHeight=800

[MeasureWebView]
Measure=Plugin
Plugin=WebView2
Url=https://www.rainmeter.net
Width=#WebWidth#
Height=#WebHeight#
X=0
Y=50
Visible=1

[MeterNavigate]
Meter=String
X=20
Y=15
FontSize=12
FontColor=100,150,255
Text="Go to Google"
LeftMouseUpAction=[!CommandMeasure MeasureWebView "Navigate https://www.google.com"]

[MeterReload]
Meter=String
X=150
Y=15
FontSize=12
FontColor=200,200,200
Text="Reload"
LeftMouseUpAction=[!CommandMeasure MeasureWebView "Reload"]
```

### Example 2: Local HTML Dashboard

```ini
[Rainmeter]
Update=1000

[MeasureWebView]
Measure=Plugin
Plugin=WebView2
Url=#@#dashboard.html
Width=800
Height=600
X=0
Y=0
Visible=1
```

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
        h1 { font-size: 3em; }
    </style>
</head>
<body>
    <h1>My Dashboard</h1>
    <p>Current time: <span id="clock"></span></p>
    <script>
        setInterval(() => {
            document.getElementById('clock').textContent = new Date().toLocaleTimeString();
        }, 1000);
    </script>
</body>
</html>
```

## 🔧 Building from Source

### Prerequisites

- Visual Studio 2022 (or 2019)
- Windows 10/11 SDK
- NuGet Package Manager

### Build Steps

1. Clone the repository:
   ```bash
   git clone https://github.com/nstechbytes/WebView2.git
   cd WebView2
   ```

2. Open `WebView2-Plugin.sln` in Visual Studio

3. Restore NuGet packages (right-click solution → Restore NuGet Packages)

4. Build the solution (Ctrl+Shift+B)

5. Find the compiled DLLs in:
   - `WebView2\x64\Release\WebView2.dll` (64-bit)
   - `WebView2\x32\Release\WebView2.dll` (32-bit)

### Build via PowerShell

```powershell
powershell -ExecutionPolicy Bypass -Command "& { . .\Build-CPP.ps1; Dist -major 1 -minor 0 -patch 0 }"
```

This creates:
- Plugin DLLs in `dist\` folder
- Complete `.rmskin` package for distribution

## 🐛 Troubleshooting

### WebView2 doesn't appear
- Ensure WebView2 Runtime is installed
- Check Rainmeter log for error messages
- Verify the skin window is visible and has appropriate dimensions

### File paths not loading
- Use `#@#` for @Resources folder: `Url=#@#mypage.html`
- Or use absolute paths: `Url=C:\MyFolder\page.html`
- The plugin automatically converts file paths to `file:///` format

### Access Denied Error
- The plugin uses TEMP directory for WebView2 data
- Ensure you have write permissions to `%TEMP%\RainmeterWebView2`

## 📝 Technical Details

- **WebView2 SDK**: Microsoft.Web.WebView2 (v1.0.2792.45)
- **Runtime**: Uses Windows Implementation Library (WIL)
- **Architecture**: Supports both x86 and x64
- **Language**: C++17
- **User Data**: Stored in `%TEMP%\RainmeterWebView2`

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the GPL-2.0 License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Microsoft Edge WebView2 team for the excellent SDK
- Rainmeter community for inspiration and support
- ModernSearchBar plugin for reference implementation

## 📧 Contact

- **Author**: nstechbytes
- **GitHub**: [https://github.com/nstechbytes/WebView2](https://github.com/nstechbytes/WebView2)

## 🔗 Related Links

- [Rainmeter Documentation](https://docs.rainmeter.net/)
- [WebView2 Documentation](https://docs.microsoft.com/microsoft-edge/webview2/)
- [Example Skins](Resources/Skins/WebView2-Plugin/)

---

**Made with ❤️ for the Rainmeter community**