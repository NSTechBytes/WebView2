let updateCount = 0;

// Called once when the plugin is ready
window.OnInitialize = function() {
    log("🚀 OnInitialize called!");
    return "Initialized!";
};

// Called on every Rainmeter update cycle
window.OnUpdate = function() {
    updateCount++;
    const message = `Update #${updateCount}`;
    
    // Update the display in HTML
    const display = document.getElementById('update-display');
    if (display) {
        display.textContent = message;
    }
    
    // Return value updates the measure's string value in Rainmeter
    return message;
};

// Function callable from Rainmeter via [Measure:CallJS('addNumbers', 'a', 'b')]
window.addNumbers = function(a, b) {
    const sum = parseInt(a) + parseInt(b);
    log(`🧮 addNumbers called: ${a} + ${b} = ${sum}`);
    return sum;
};

// Helper to log to HTML
function log(message) {
    const container = document.getElementById('log-container');
    if (container) {
        const entry = document.createElement('div');
        entry.className = 'log-entry';
        entry.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
        container.insertBefore(entry, container.firstChild);
        
        // Keep log size manageable
        if (container.children.length > 50) {
            container.removeChild(container.lastChild);
        }
    }
    console.log(message);
}
