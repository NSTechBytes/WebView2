function updateClock() {
    const now = new Date();
    
    let hours = now.getHours();
    const minutes = now.getMinutes();
    const seconds = now.getSeconds();
    const ampm = hours >= 12 ? 'PM' : 'AM';
    
    // Convert to 12-hour format
    hours = hours % 12;
    hours = hours ? hours : 12; // the hour '0' should be '12'
    
    // Pad with leading zeros
    const hoursStr = hours.toString().padStart(2, '0');
    const minutesStr = minutes.toString().padStart(2, '0');
    const secondsStr = seconds.toString().padStart(2, '0');
    
    // Update DOM
    document.getElementById('hours').textContent = hoursStr;
    document.getElementById('minutes').textContent = minutesStr;
    document.getElementById('seconds').textContent = secondsStr;
    document.getElementById('ampm').textContent = ampm;
    
    // Update Date
    const options = { weekday: 'long', month: 'long', day: 'numeric' };
    const dateStr = now.toLocaleDateString('en-US', options);
    document.getElementById('date').textContent = dateStr;
}

// Update immediately and then every second
updateClock();
setInterval(updateClock, 1000);
