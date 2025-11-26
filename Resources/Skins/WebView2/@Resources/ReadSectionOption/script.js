async function refreshValues() {
    // 1. Read Variable (String)
    updateField('var', async () => {

        return await RainmeterAPI.ReadStringFromSection('Variables', 'TestVar', 'N/A');
    });

    // 2. Read Measure String
    updateField('measure', async () => {
        return await RainmeterAPI.ReadStringFromSection('MeasureTest', 'String', 'N/A');
    });

    // 3. Read Measure Int (Int)
    updateField('measure-int', async () => {
        const val = await RainmeterAPI.ReadIntFromSection('MeasureCalc', 'Formula', 0); 
        return val;
    });

    // 5. Read Variable Double (Double)
    updateField('var-double', async () => {
        const val = await RainmeterAPI.ReadDoubleFromSection('Variables', 'DoubleExample', 0); 
        return val;
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
        document.querySelectorAll('.status').forEach(el => {
            el.textContent = 'API Not Found';
            el.className = 'status error';
        });
    }
});