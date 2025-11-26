(function() {
    'use strict';

    // DOM Elements
    const searchInput = document.getElementById('search-input');
    const tocList = document.getElementById('toc-list');
    const noResults = document.getElementById('no-results');
    const sections = document.querySelectorAll('section');
    const menuToggle = document.getElementById('menu-toggle');
    const sidebar = document.getElementById('sidebar');

    // State
    let searchDebounceTimer;
    const DEBOUNCE_DELAY = 300;
    let originalContent = new Map(); // Store original HTML to restore after highlighting

    // Initialize
    function init() {
        // Store original content for restoring after search clears
        sections.forEach(section => {
            originalContent.set(section.id, section.innerHTML);
        });

        // Event Listeners
        searchInput.addEventListener('input', handleSearchInput);
        menuToggle.addEventListener('click', toggleSidebar);
        
        // Close sidebar when clicking outside on mobile
        document.addEventListener('click', (e) => {
            if (window.innerWidth <= 768 && 
                sidebar.classList.contains('open') && 
                !sidebar.contains(e.target) && 
                !menuToggle.contains(e.target)) {
                toggleSidebar();
            }
        });

        // Handle TOC link clicks for smooth scrolling and mobile menu closing
        tocList.querySelectorAll('a').forEach(link => {
            link.addEventListener('click', (e) => {
                if (window.innerWidth <= 768) {
                    toggleSidebar();
                }
            });
        });
    }

    // Search Logic
    function handleSearchInput(e) {
        clearTimeout(searchDebounceTimer);
        const query = e.target.value.trim().toLowerCase();

        searchDebounceTimer = setTimeout(() => {
            performSearch(query);
        }, DEBOUNCE_DELAY);
    }

    function performSearch(query) {
        let hasResults = false;
        
        // Reset if empty
        if (!query) {
            resetSearch();
            return;
        }

        sections.forEach(section => {
            const sectionId = section.id;
            const originalHTML = originalContent.get(sectionId);
            const textContent = section.textContent.toLowerCase();
            const tocLink = tocList.querySelector(`a[href="#${sectionId}"]`);
            const tocItem = tocLink.parentElement;

            if (textContent.includes(query)) {
                // Match found
                hasResults = true;
                section.classList.remove('hidden');
                if (tocItem) tocItem.classList.remove('hidden');

                // Highlight matches
                section.innerHTML = highlightMatches(originalHTML, query);
            } else {
                // No match in this section
                section.classList.add('hidden');
                if (tocItem) tocItem.classList.add('hidden');
            }
        });

        // Toggle "No results" message
        if (hasResults) {
            noResults.classList.add('hidden');
        } else {
            noResults.classList.remove('hidden');
        }
    }

    function highlightMatches(html, query) {
        // Simple regex-based highlighting that avoids HTML tags
        // Note: This is a basic implementation. For production with complex HTML, 
        // a tree walker or DOM manipulation approach is safer.
        const regex = new RegExp(`(${escapeRegExp(query)})`, 'gi');
        
        // We need to be careful not to replace text inside HTML tags.
        // This simple version assumes text content is mostly safe to replace 
        // if we avoid tag brackets.
        
        // A safer way for this specific requirement without libraries:
        const tempDiv = document.createElement('div');
        tempDiv.innerHTML = html;
        
        const walker = document.createTreeWalker(
            tempDiv, 
            NodeFilter.SHOW_TEXT, 
            null, 
            false
        );

        let node;
        const nodesToReplace = [];

        while (node = walker.nextNode()) {
            if (node.nodeValue.toLowerCase().includes(query)) {
                nodesToReplace.push(node);
            }
        }

        nodesToReplace.forEach(node => {
            const span = document.createElement('span');
            span.innerHTML = node.nodeValue.replace(regex, '<mark>$1</mark>');
            node.parentNode.replaceChild(span, node);
        });

        return tempDiv.innerHTML;
    }

    function resetSearch() {
        sections.forEach(section => {
            section.classList.remove('hidden');
            section.innerHTML = originalContent.get(section.id);
        });
        
        tocList.querySelectorAll('li').forEach(li => li.classList.remove('hidden'));
        noResults.classList.add('hidden');
    }

    function escapeRegExp(string) {
        return string.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
    }

    // Sidebar Logic
    function toggleSidebar() {
        const isOpen = sidebar.classList.toggle('open');
        menuToggle.setAttribute('aria-expanded', isOpen);
    }

    // Run
    init();
    initCopyButtons();

    function initCopyButtons() {
        // Target ONLY pre blocks (standard code blocks)
        // No buttons in tables
        const codeBlocks = document.querySelectorAll('pre');

        codeBlocks.forEach(block => {
            // Create wrapper
            const wrapper = document.createElement('div');
            wrapper.className = 'code-wrapper';
            
            // For table cells, we need to be careful not to break layout
            if (block.tagName === 'CODE') {
                wrapper.style.display = 'inline-block';
                wrapper.style.width = '100%';
            }
            
            // Insert wrapper before block
            block.parentNode.insertBefore(wrapper, block);
            
            // Move block into wrapper
            wrapper.appendChild(block);
            
            // Create copy button
            const button = document.createElement('button');
            button.className = 'copy-btn';
            button.textContent = 'Copy';
            button.ariaLabel = 'Copy code to clipboard';
            
            // Add click handler
            button.addEventListener('click', (e) => {
                e.stopPropagation(); // Prevent triggering other clicks
                const code = block.textContent;
                
                navigator.clipboard.writeText(code.trim()).then(() => {
                    button.textContent = 'Copied!';
                    button.classList.add('copied');
                    
                    setTimeout(() => {
                        button.textContent = 'Copy';
                        button.classList.remove('copied');
                    }, 2000);
                }).catch(err => {
                    console.error('Failed to copy:', err);
                    button.textContent = 'Error';
                });
            });
            
            wrapper.appendChild(button);
        });
    }

})();
