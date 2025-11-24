// External JavaScript file for WebView2 demo
// This file demonstrates various JavaScript capabilities

// Initialize when DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
    console.log('External script loaded successfully!');
    initializeApp();
});

// Main initialization function
function initializeApp() {
    // Update clock every second
    updateClock();
    setInterval(updateClock, 1000);
    
    // Add event listeners
    setupEventListeners();
    
    // Initialize any charts or visualizations
    initializeVisuals();
}

// Clock update function
function updateClock() {
    const clockElement = document.getElementById('clock');
    if (clockElement) {
        const now = new Date();
        const timeString = now.toLocaleTimeString('en-US', {
            hour: '2-digit',
            minute: '2-digit',
            second: '2-digit',
            hour12: true
        });
        const dateString = now.toLocaleDateString('en-US', {
            weekday: 'long',
            year: 'numeric',
            month: 'long',
            day: 'numeric'
        });
        clockElement.innerHTML = `
            <div class="time">${timeString}</div>
            <div class="date">${dateString}</div>
        `;
    }
}

// Setup event listeners
function setupEventListeners() {
    // Color change button
    const colorBtn = document.getElementById('changeColorBtn');
    if (colorBtn) {
        colorBtn.addEventListener('click', changeBackgroundColor);
    }
    
    // Theme toggle button
    const themeBtn = document.getElementById('toggleThemeBtn');
    if (themeBtn) {
        themeBtn.addEventListener('click', toggleTheme);
    }
    
    // Animation button
    const animBtn = document.getElementById('animateBtn');
    if (animBtn) {
        animBtn.addEventListener('click', triggerAnimation);
    }
}

// Change background color randomly
function changeBackgroundColor() {
    const colors = [
        'linear-gradient(135deg, #667eea 0%, #764ba2 100%)',
        'linear-gradient(135deg, #f093fb 0%, #f5576c 100%)',
        'linear-gradient(135deg, #4facfe 0%, #00f2fe 100%)',
        'linear-gradient(135deg, #43e97b 0%, #38f9d7 100%)',
        'linear-gradient(135deg, #fa709a 0%, #fee140 100%)',
        'linear-gradient(135deg, #30cfd0 0%, #330867 100%)'
    ];
    const randomColor = colors[Math.floor(Math.random() * colors.length)];
    document.body.style.background = randomColor;
    
    // Show notification
    showNotification('Background color changed!');
}

// Toggle between light and dark theme
let isDarkTheme = true;
function toggleTheme() {
    isDarkTheme = !isDarkTheme;
    
    if (isDarkTheme) {
        document.body.style.background = 'linear-gradient(135deg, #667eea 0%, #764ba2 100%)';
        document.body.style.color = '#ffffff';
    } else {
        document.body.style.background = 'linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%)';
        document.body.style.color = '#333333';
    }
    
    showNotification(`Switched to ${isDarkTheme ? 'dark' : 'light'} theme`);
}

// Trigger animation
function triggerAnimation() {
    const container = document.querySelector('.container');
    if (container) {
        container.style.transform = 'scale(0.95)';
        setTimeout(() => {
            container.style.transform = 'scale(1)';
        }, 200);
    }
    
    showNotification('Animation triggered!');
}

// Show notification
function showNotification(message) {
    // Create notification element
    const notification = document.createElement('div');
    notification.className = 'notification';
    notification.textContent = message;
    notification.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        background: rgba(0, 0, 0, 0.8);
        color: white;
        padding: 15px 25px;
        border-radius: 8px;
        font-size: 14px;
        z-index: 1000;
        animation: slideIn 0.3s ease;
    `;
    
    document.body.appendChild(notification);
    
    // Remove after 3 seconds
    setTimeout(() => {
        notification.style.animation = 'slideOut 0.3s ease';
        setTimeout(() => {
            document.body.removeChild(notification);
        }, 300);
    }, 3000);
}

// Initialize visualizations
function initializeVisuals() {
    // Add any chart or visualization initialization here
    console.log('Visuals initialized');
}

// Function callable from Rainmeter
function rainmeterCommand(command, data) {
    console.log('Rainmeter command received:', command, data);
    
    switch(command) {
        case 'setColor':
            document.body.style.background = data;
            break;
        case 'showMessage':
            showNotification(data);
            break;
        case 'updateData':
            updateDashboardData(data);
            break;
        default:
            console.log('Unknown command:', command);
    }
}

// Update dashboard with data from Rainmeter
function updateDashboardData(data) {
    const dataElement = document.getElementById('dashboardData');
    if (dataElement) {
        dataElement.textContent = JSON.stringify(data, null, 2);
    }
}

// Add CSS animations
const style = document.createElement('style');
style.textContent = `
    @keyframes slideIn {
        from {
            transform: translateX(100%);
            opacity: 0;
        }
        to {
            transform: translateX(0);
            opacity: 1;
        }
    }
    
    @keyframes slideOut {
        from {
            transform: translateX(0);
            opacity: 1;
        }
        to {
            transform: translateX(100%);
            opacity: 0;
        }
    }
    
    .container {
        transition: transform 0.2s ease;
    }
`;
document.head.appendChild(style);

// Export functions for use in HTML
window.rainmeterCommand = rainmeterCommand;
window.changeBackgroundColor = changeBackgroundColor;
window.toggleTheme = toggleTheme;
window.triggerAnimation = triggerAnimation;

// Alias for example.html compatibility
window.changeColor = changeBackgroundColor;

// Show alert function for example.html
window.showAlert = function() {
    alert('Hello from WebView2! 🎉\n\nThis demonstrates JavaScript execution in the WebView2 control.');
};
