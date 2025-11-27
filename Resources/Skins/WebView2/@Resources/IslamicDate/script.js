// Islamic Date Widget Script using Aladhan API

// Islamic month names
const islamicMonths = [
    'Muharram',
    'Safar',
    'Rabi\' al-Awwal',
    'Rabi\' al-Thani',
    'Jumada al-Awwal',
    'Jumada al-Thani',
    'Rajab',
    'Sha\'ban',
    'Ramadan',
    'Shawwal',
    'Dhu al-Qi\'dah',
    'Dhu al-Hijjah'
];

// Fetch Islamic date from API
async function fetchIslamicDate() {
    try {
        // Get current date
        const now = new Date();
        const day = now.getDate();
        const month = now.getMonth() + 1; // JavaScript months are 0-indexed
        const year = now.getFullYear();
        
        // Fetch from Aladhan API with Pakistan calculation method
        // method=1 uses University of Islamic Sciences, Karachi (Pakistan)
        const response = await fetch(
            `https://api.aladhan.com/v1/gToH/${day}-${month}-${year}?adjustment=1`,
            {
                method: 'GET',
                mode: 'cors',
                cache: 'no-cache',
                credentials: 'omit'
            }
        );
        
        if (!response.ok) {
            throw new Error('Failed to fetch Islamic date');
        }
        
        const data = await response.json();
        
        if (data.code === 200 && data.data && data.data.hijri) {
            const hijri = data.data.hijri;
            
            // Update UI
            document.getElementById('hijri-day').textContent = hijri.day;
            document.getElementById('hijri-month').textContent = hijri.month.en;
            document.getElementById('hijri-year').textContent = `${hijri.year} AH`;
            
            console.log('Islamic date updated:', hijri.day, hijri.month.en, hijri.year);
        } else {
            throw new Error('Invalid API response');
        }
        
    } catch (error) {
        console.error('Error fetching Islamic date:', error);
        // Fallback to showing error or default values
        document.getElementById('hijri-day').textContent = '--';
        document.getElementById('hijri-month').textContent = 'Error';
        document.getElementById('hijri-year').textContent = '---- AH';
    }
}

// Initialize
function init() {
    // Initial fetch
    fetchIslamicDate();
    
    // Update once per day at midnight
    const now = new Date();
    const tomorrow = new Date(now.getFullYear(), now.getMonth(), now.getDate() + 1);
    const msUntilMidnight = tomorrow - now;
    
    // Set timeout for midnight, then update daily
    setTimeout(() => {
        fetchIslamicDate();
        // Update every 24 hours after that
        setInterval(fetchIslamicDate, 24 * 60 * 60 * 1000);
    }, msUntilMidnight);
}

// Wait for DOM to be ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}
