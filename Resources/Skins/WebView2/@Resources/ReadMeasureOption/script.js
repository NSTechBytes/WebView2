async function refreshValues() {
    // 1. Read String
    updateField('string', async () => {
        return await RainmeterAPI.ReadString('TestString', 'Default');
    });

    // 2. Read Int
    updateField('int', async () => {
        return await RainmeterAPI.ReadInt('TestInt', 0);
    });

    // 3. Read Double
    updateField('double', async () => {
        return await RainmeterAPI.ReadDouble('TestDouble', 0.0);
    });

    // 4. Read Formula
    updateField('formula', async () => {
        return await RainmeterAPI.ReadFormula('TestFormula', 0);
    });

    // 5. Read Path
    updateField('path', async () => {
        return await RainmeterAPI.ReadPath('TestPath', '');
    });
}

async function updateField(id, fetchFn) {
    const output = document.getElementById(`${id}-output`);
    const status = document.getElementById(`${id}-status`);

    try {
        status.textContent = 'Reading...';
        status.className = 'status loading';

        const value = await fetchFn();

        output.textContent = value;
        status.textContent = 'Updated';
        status.className = 'status success';
    } catch (e) {
        console.error(e);
        output.textContent = 'Error';
        status.textContent = e.message;
        status.className = 'status error';
    }
}

// Initial load
window.addEventListener('DOMContentLoaded', () => {
    if (window.RainmeterAPI) {
        refreshValues();
    } else {
        // Fallback or wait for event if needed, though DOMContentLoaded usually suffices if API is injected early
        // But for safety with some injection methods:
        window.addEventListener('RainmeterAPIReady', refreshValues);
        
        // Also set initial status
        document.querySelectorAll('.status').forEach(el => {
            el.textContent = 'Waiting for API...';
            el.className = 'status loading';
        });
    }
});
