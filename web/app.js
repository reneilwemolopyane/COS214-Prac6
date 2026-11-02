/* 
   TIVIDY Web UI — app.js
   */

const API = '';

let state = {
    instances: [],
    selectedInstance: null,
    definitions: [],
    activeTab: 'timeline',
    sideEffectsCache: null,
    reportsCache: null,
    currentCases: [],
    selectedCaseIndex: -1
};

/* 
   Outcome options per work item.
   Items not listed here default to ["done"].
    */
const OUTCOME_OPTIONS = {
    'Review claim and decide': ['REPAIR', 'WRITE_OFF'],
    'Screen claim for fraud': ['CLEAR', 'SUSPICIOUS'],
};

/* 
   Modal system — replaces alert / prompt / confirm
   */

function closeModal() {
    const root = document.getElementById('modal-root');
    root.innerHTML = '';
}

function showModal({ title, bodyHTML, actions, width = 'max-w-md' }) {
    return new Promise((resolve) => {
        const root = document.getElementById('modal-root');
        root.innerHTML = `
            <div class="fixed inset-0 z-[100] flex items-center justify-center p-4
                        bg-slate-900/40 backdrop-blur-sm" id="modal-backdrop">
                <div class="modal-enter bg-white rounded-2xl shadow-xl border border-slate-200
                            w-full ${width} max-h-[85vh] overflow-y-auto">
                    <div class="px-6 pt-5 pb-2">
                        <h2 class="font-display text-lg font-bold text-slate-900">${title}</h2>
                    </div>
                    <div class="px-6 pb-4">${bodyHTML}</div>
                    <div class="px-6 pb-5 flex justify-end gap-2 flex-wrap" id="modal-actions"></div>
                </div>
            </div>
        `;

        const actionsDiv = document.getElementById('modal-actions');
        actions.forEach((act) => {
            const btn = document.createElement('button');
            btn.type = 'button';
            btn.textContent = act.label;
            const base = 'px-4 py-2 rounded-lg text-sm font-semibold transition-colors cursor-pointer';
            const variants = {
                primary:   'text-white bg-brand hover:bg-brand-hover',
                secondary: 'text-slate-700 bg-white border border-slate-200 hover:bg-slate-50',
                danger:    'text-white bg-red-600 hover:bg-red-700',
            };
            btn.className = `${base} ${variants[act.style || 'secondary']}`;
            btn.onclick = () => {
                let value = act.value;
                if (act.collect) value = act.collect();
                if (act.validate) {
                    const err = act.validate(value);
                    if (err) {
                        const errDiv = document.getElementById('modal-error');
                        if (errDiv) { errDiv.textContent = err; errDiv.classList.remove('hidden'); }
                        return;
                    }
                }
                closeModal();
                resolve(value);
            };
            actionsDiv.appendChild(btn);
        });

        // Click backdrop to cancel
        document.getElementById('modal-backdrop').addEventListener('click', (e) => {
            if (e.target.id === 'modal-backdrop') { closeModal(); resolve(null); }
        });

        // Focus the first input if present
        const firstInput = root.querySelector('input, textarea, select');
        if (firstInput) firstInput.focus();
    });
}

/** Simple alert replacement. */
function showAlert(title, message) {
    return showModal({
        title,
        bodyHTML: `<p class="text-sm text-slate-600 leading-relaxed">${message}</p>`,
        actions: [{ label: 'OK', value: true, style: 'primary' }],
    });
}

/** Confirmation replacement. */
function showConfirm(title, message) {
    return showModal({
        title,
        bodyHTML: `<p class="text-sm text-slate-600 leading-relaxed">${message}</p>`,
        actions: [
            { label: 'Cancel', value: false, style: 'secondary' },
            { label: 'Confirm', value: true, style: 'primary' },
        ],
    });
}

/** Outcome picker — replaces prompt() for complete actions. */
function showOutcomePicker(itemName) {
    const options = OUTCOME_OPTIONS[itemName] || ['done'];

    const bodyHTML = `
        <p class="text-sm text-slate-500 mb-4">
            Choose the outcome for <strong class="text-slate-800">${itemName}</strong>.
        </p>
        <div class="flex flex-col gap-2" id="outcome-options">
            ${options.map((opt) => `
                <button type="button"
                        class="outcome-btn w-full text-left px-4 py-3 rounded-xl border-2
                               border-slate-200 bg-white hover:border-brand hover:bg-brand-light
                               transition-colors cursor-pointer font-medium text-slate-800"
                        data-outcome="${opt}">
                    ${opt}
                </button>
            `).join('')}
        </div>
    `;

    return showModal({
        title: 'Complete work item',
        bodyHTML,
        actions: [{ label: 'Cancel', value: null, style: 'secondary' }],
    }).then((val) => {
        // The button click resolves before the modal closes; we need to
        // intercept the outcome buttons manually.
        return val; // handled below
    });
}

/** Reason input — replaces prompt() for reject / escalate. */
function showReasonInput(action, defaultValue) {
    return showModal({
        title: action === 'reject' ? 'Reject work item' : 'Escalate work item',
        bodyHTML: `
            <p class="text-sm text-slate-500 mb-3">
                ${action === 'reject'
                    ? 'Give a reason for rejecting this work item.'
                    : 'Give a reason for escalating this work item.'}
            </p>
            <textarea id="reason-input" rows="3"
                      class="w-full rounded-xl border border-slate-200 px-3 py-2 text-sm
                             focus:outline-none focus:ring-2 focus:ring-brand/30 focus:border-brand
                             resize-none"
                      placeholder="Enter a reason...">${defaultValue}</textarea>
            <p id="modal-error" class="hidden text-xs text-red-600 mt-1"></p>
        `,
        actions: [
            { label: 'Cancel', value: null, style: 'secondary' },
            {
                label: action === 'reject' ? 'Reject' : 'Escalate',
                style: action === 'reject' ? 'danger' : 'primary',
                collect: () => document.getElementById('reason-input').value.trim(),
                validate: (v) => (!v ? 'A reason is required.' : null),
            },
        ],
    });
}

/* 
   Toast
   */

function showToast(message, type = 'info') {
    const root = document.getElementById('toast-root');
    const colors = {
        info:    'bg-slate-800 text-white',
        success: 'bg-emerald-600 text-white',
        error:   'bg-red-600 text-white',
    };
    const toast = document.createElement('div');
    toast.className = `${colors[type]} px-4 py-2.5 rounded-xl shadow-lg text-sm font-medium
                       transition-opacity duration-300 opacity-0`;
    toast.textContent = message;
    root.appendChild(toast);
    requestAnimationFrame(() => { toast.style.opacity = '1'; });
    setTimeout(() => {
        toast.style.opacity = '0';
        setTimeout(() => toast.remove(), 300);
    }, 3000);
}

/* 
   Boot
   */

async function init() {
    try {
        const [defs, insts] = await Promise.all([
            fetch(`${API}/api/definitions`).then((r) => r.json()),
            fetch(`${API}/api/instances`).then((r) => r.json()),
        ]);
        state.definitions = defs.definitions || [];
        state.instances = insts.instances || [];
        renderDashboard();
    } catch (e) {
        document.getElementById('main').innerHTML = `
            <div class="bg-red-50 border border-red-200 text-red-700 px-4 py-3 rounded-xl text-sm">
                Failed to connect to engine backend: ${e.message}
            </div>
        `;
    }
}

function renderView() {
    init();
}

/* 
   Dashboard
   */

function renderDashboard() {
    const main = document.getElementById('main');

    main.innerHTML = `
        <div class="flex items-center justify-between mb-6 flex-wrap gap-4">
            <div>
                <h1 class="font-display text-2xl font-bold text-slate-900">Instances</h1>
                <p class="text-sm text-slate-500 mt-0.5">Running workflows across all industries.</p>
            </div>
        </div>

        <div id="outages-panel" class="hidden mb-6"></div>

        <div class="bg-white rounded-2xl border border-slate-200 shadow-sm overflow-hidden">
            <table class="w-full text-left">
                <thead>
                    <tr class="border-b border-slate-200 bg-slate-50/60">
                        <th class="px-5 py-3 text-xs font-semibold text-slate-500 uppercase tracking-wider">Instance</th>
                        <th class="px-5 py-3 text-xs font-semibold text-slate-500 uppercase tracking-wider">Scenario</th>
                        <th class="px-5 py-3 text-xs font-semibold text-slate-500 uppercase tracking-wider">Status</th>
                        <th class="px-5 py-3 text-xs font-semibold text-slate-500 uppercase tracking-wider text-right">Action</th>
                    </tr>
                </thead>
                <tbody>
                    ${state.instances.length === 0 ? `
                        <tr>
                            <td colspan="4" class="px-5 py-12 text-center text-sm text-slate-400 italic">
                                No instances running. Start a new workflow to begin.
                            </td>
                        </tr>
                    ` : state.instances.map((i) => `
                        <tr class="hover-row border-b border-slate-100 last:border-0 transition-colors">
                            <td class="px-5 py-3.5">
                                <code class="text-sm font-mono text-slate-800 bg-slate-100 px-2 py-0.5 rounded">${i.id}</code>
                            </td>
                            <td class="px-5 py-3.5 text-sm text-slate-600">${i.scenarioId}</td>
                            <td class="px-5 py-3.5">${statusPill(i.outcome)}</td>
                            <td class="px-5 py-3.5 text-right">
                                <button onclick="viewInstance('${i.id}')"
                                        class="text-sm font-semibold text-brand hover:text-brand-hover
                                               transition-colors cursor-pointer">
                                    View
                                </button>
                            </td>
                        </tr>
                    `).join('')}
                </tbody>
            </table>
        </div>
    `;
}

function statusPill(status) {
    const map = {
        CREATED:    'bg-slate-100 text-slate-600',
        RUNNING:    'bg-blue-50 text-blue-700',
        COMPLETED:  'bg-emerald-50 text-emerald-700',
        REJECTED:   'bg-red-50 text-red-700',
        CANCELLED:  'bg-slate-100 text-slate-500',
        CLOSED:     'bg-slate-100 text-slate-500',
        IN_PROGRESS:'bg-amber-50 text-amber-700',
        REGISTERED: 'bg-slate-100 text-slate-600',
        NOT_FOUND:  'bg-slate-100 text-slate-400',
    };
    const cls = map[status] || 'bg-slate-100 text-slate-600';
    return `<span class="inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-semibold ${cls}">${status}</span>`;
}

/* 
   Top-bar actions: advance time + systems
   */

document.getElementById('btn-advance').onclick = async () => {
    const days = await showModal({
        title: 'Advance simulated time',
        bodyHTML: `
            <p class="text-sm text-slate-500 mb-3">
                Move the simulated clock forward to trigger deadline escalations.
            </p>
            <input id="days-input" type="number" min="1" value="1"
                   class="w-full rounded-xl border border-slate-200 px-3 py-2 text-sm
                          focus:outline-none focus:ring-2 focus:ring-brand/30 focus:border-brand">
        `,
        actions: [
            { label: 'Cancel', value: null, style: 'secondary' },
            {
                label: 'Advance',
                style: 'primary',
                collect: () => parseInt(document.getElementById('days-input').value, 10) || 1,
            },
        ],
    });
    if (days === null) return;

    try {
        const res = await fetch(`${API}/api/engine/advance-clock`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ days }),
        }).then((r) => r.json());

        if (res.ok) {
            showToast(res.escalated ? `${res.escalated} item(s) escalated.` : 'No deadlines expired.',
                      res.escalated ? 'success' : 'info');
            init();
        } else {
            showAlert('Error', res.error || 'Unknown error');
        }
    } catch (e) {
        showAlert('Network error', e.message);
    }
};

document.getElementById('btn-systems').onclick = () => toggleOutagesPanel();

async function toggleOutagesPanel() {
    const panel = document.getElementById('outages-panel');
    if (!panel) return;

    // If on an instance detail page, go back to dashboard first
    if (!document.querySelector('#main table')) {
        init();
        setTimeout(() => toggleOutagesPanel(), 200);
        return;
    }

    if (!panel.classList.contains('hidden')) {
        panel.classList.add('hidden');
        panel.innerHTML = '';
        return;
    }

    panel.classList.remove('hidden');
    panel.innerHTML = `<div class="text-sm text-slate-400 italic">Loading systems...</div>`;

    let html = '';
    for (const d of state.definitions) {
        try {
            const r = await fetch(`${API}/api/scenarios/${d.id}/systems`).then((x) => x.json());
            if (!r.ok) continue;

            html += `
                <div class="bg-white rounded-2xl border border-slate-200 shadow-sm p-4 mb-3">
                    <h3 class="font-display text-sm font-bold text-slate-800 mb-3">${d.title}</h3>
                    <div class="flex flex-wrap gap-2">
                        ${r.systems.map((sys) => `
                            <button class="sys-toggle px-3 py-1.5 rounded-lg text-xs font-medium
                                           border border-slate-200 bg-white text-slate-600
                                           hover:bg-slate-50 transition-colors cursor-pointer"
                                    data-scenario="${d.id}"
                                    data-system="${sys}"
                                    data-down="false"
                                    onclick="toggleOutage(this)">
                                ${sys}: UP
                            </button>
                        `).join('')}
                    </div>
                </div>
            `;
        } catch (e) { /* skip */ }
    }

    panel.innerHTML = html || `
        <div class="bg-white rounded-2xl border border-slate-200 shadow-sm p-4 text-sm text-slate-400 italic">
            No systems configured.
        </div>
    `;
}

async function toggleOutage(btn) {
    const scenario = btn.dataset.scenario;
    const system = btn.dataset.system;
    const newDown = btn.dataset.down !== 'true';

    try {
        const res = await fetch(`${API}/api/scenarios/${scenario}/systems/${system}/outage`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ down: newDown }),
        }).then((r) => r.json());

        if (res.ok) {
            btn.dataset.down = newDown ? 'true' : 'false';
            btn.textContent = `${system}: ${newDown ? 'DOWN' : 'UP'}`;
            btn.className = newDown
                ? 'sys-toggle px-3 py-1.5 rounded-lg text-xs font-medium border border-red-200 bg-red-50 text-red-700 transition-colors cursor-pointer'
                : 'sys-toggle px-3 py-1.5 rounded-lg text-xs font-medium border border-slate-200 bg-white text-slate-600 hover:bg-slate-50 transition-colors cursor-pointer';
        } else {
            showAlert('Error', res.error || 'Failed to toggle system');
        }
    } catch (e) {
        showAlert('Network error', e.message);
    }
}

/* 
   Instance detail
   */

async function viewInstance(id) {
    const res = await fetch(`${API}/api/instances/${id}`).then((r) => r.json());
    if (!res.ok) return showAlert('Error', 'Failed to load instance');

    state.selectedInstance = res.instance;
    state.activeTab = 'timeline';
    state.sideEffectsCache = null;
    state.reportsCache = null;

    const inst = state.selectedInstance;
    const main = document.getElementById('main');

    main.innerHTML = `
        <div class="flex items-start justify-between mb-6 flex-wrap gap-4">
            <div class="min-w-0">
                <div class="flex items-center gap-3 flex-wrap">
                    <h1 class="font-display text-2xl font-bold text-slate-900">
                        Instance
                        <code class="text-base font-mono font-normal text-slate-600 bg-slate-100 px-2 py-0.5 rounded ml-1">${inst.id}</code>
                    </h1>
                    ${statusPill(inst.status)}
                </div>
                <p class="text-sm text-slate-500 mt-1">Version ${inst.version || 1}</p>
            </div>
            <div class="flex items-center gap-2 flex-wrap">
                <button onclick="doUndo('${inst.id}')"
                        class="px-3.5 py-2 rounded-lg text-sm font-medium text-slate-600 bg-white
                               border border-slate-200 hover:bg-slate-50 transition-colors cursor-pointer">
                    Undo Step
                </button>
                <button onclick="doCancel('${inst.id}')"
                        class="px-3.5 py-2 rounded-lg text-sm font-medium text-red-600 bg-white
                               border border-red-200 hover:bg-red-50 transition-colors cursor-pointer">
                    Cancel
                </button>
                <button onclick="init()"
                        class="px-3.5 py-2 rounded-lg text-sm font-medium text-slate-600 bg-white
                               border border-slate-200 hover:bg-slate-50 transition-colors cursor-pointer">
                    Back
                </button>
            </div>
        </div>

        <div class="grid grid-cols-1 lg:grid-cols-[minmax(340px,420px)_1fr_minmax(320px,380px)] gap-5 min-w-0">

            <!-- Left: tree + data -->
            <div class="space-y-5 min-w-0">
                <div class="bg-white rounded-2xl border border-slate-200 shadow-sm p-5">
                    <h2 class="font-display text-sm font-bold text-slate-900 uppercase tracking-wide mb-4">Structure</h2>
                    <div id="tree-container"></div>
                </div>
                <div class="bg-white rounded-2xl border border-slate-200 shadow-sm p-5">
                    <h2 class="font-display text-sm font-bold text-slate-900 uppercase tracking-wide mb-4">Instance Data</h2>
                    <div id="data-container" class="space-y-1.5 text-sm"></div>
                </div>
            </div>

            <!-- Center: actions -->
            <div class="bg-white rounded-2xl border border-slate-200 shadow-sm p-5 min-w-0">
                <h2 class="font-display text-sm font-bold text-slate-900 uppercase tracking-wide mb-4">Item Details</h2>
                <div id="action-container" class="text-sm text-slate-400 italic">
                    Select a work item from the structure to view actions.
                </div>
            </div>

            <!-- Right: tabs -->
            <div class="bg-white rounded-2xl border border-slate-200 shadow-sm p-5 min-w-0">
                <div class="flex gap-1 border-b border-slate-200 mb-4 overflow-x-auto">
                    <button class="tab-btn active" data-tab="timeline"    onclick="switchTab('timeline')">Timeline</button>
                    <button class="tab-btn"        data-tab="sideEffects" onclick="switchTab('sideEffects')">Side Effects</button>
                    <button class="tab-btn"        data-tab="reports"     onclick="switchTab('reports')">Reports</button>
                </div>
                <div id="tab-content" class="max-h-[500px] overflow-y-auto text-sm"></div>
            </div>
        </div>
    `;

    renderTree(inst.tree);
    renderDataPanel(inst.data);
    renderTabContent();
}

/* 
   Instance data panel
   */

function renderDataPanel(data) {
    const container = document.getElementById('data-container');
    if (!container) return;

    const keys = Object.keys(data || {}).sort();
    if (keys.length === 0) {
        container.innerHTML = `<span class="text-slate-400 italic">No data yet.</span>`;
        return;
    }

    container.innerHTML = keys.map((k) => `
        <div class="flex flex-col gap-0.5 py-1 border-b border-slate-100 last:border-0">
            <span class="text-xs text-slate-500">${k}</span>
            <code class="text-xs text-slate-800 break-all">${data[k]}</code>
        </div>
    `).join('');
}

/* 
   Tabs
   */

function switchTab(name) {
    state.activeTab = name;
    document.querySelectorAll('.tab-btn').forEach((b) =>
        b.classList.toggle('active', b.dataset.tab === name));
    renderTabContent();
}

async function renderTabContent() {
    const container = document.getElementById('tab-content');
    if (!container) return;
    const inst = state.selectedInstance;

    if (state.activeTab === 'timeline') {
        if (!inst.events || inst.events.length === 0) {
            container.innerHTML = `<p class="text-slate-400 italic">No events yet.</p>`;
            return;
        }
        container.innerHTML = inst.events.map((e) => `
            <div class="border-l-2 border-brand pl-3 py-1.5 mb-3 relative">
                <div class="flex items-center gap-2 flex-wrap">
                    <strong class="text-slate-800 text-xs font-semibold">${e.typeName || e.type}</strong>
                    ${e.itemId ? `<code class="text-[11px] text-slate-500 bg-slate-100 px-1.5 py-0.5 rounded">${e.itemId}</code>` : ''}
                </div>
                <p class="text-xs text-slate-500 mt-0.5 leading-relaxed">${e.text || e.details || ''}</p>
            </div>
        `).join('');
        return;
    }

    if (state.activeTab === 'sideEffects') {
        if (!state.sideEffectsCache) {
            container.innerHTML = `<div class="text-slate-400 italic">Loading...</div>`;
            try {
                state.sideEffectsCache = await fetch(`${API}/api/instances/${inst.id}/side-effects`)
                    .then((r) => r.json());
            } catch (e) {
                container.innerHTML = `<div class="text-red-600 text-sm">Failed to load.</div>`;
                return;
            }
        }
        const se = state.sideEffectsCache;

        const list = (title, arr, danger) => `
            <h3 class="font-display text-xs font-bold text-slate-900 uppercase tracking-wide mb-2 mt-4">${title}</h3>
            ${(!arr || arr.length === 0)
                ? `<p class="text-slate-400 italic text-xs">none</p>`
                : arr.map((x) => `
                    <div class="border-l-2 ${danger ? 'border-red-400' : 'border-slate-200'} pl-3 py-1 mb-2 text-xs text-slate-600">
                        ${x}
                    </div>
                `).join('')}
        `;

        const cmdList = (se.commands || []).map((c) =>
            `${c.name} ${c.itemId} [${c.status}, ${c.attempts} attempt(s)]${c.error ? ' — ' + c.error : ''}`);

        container.innerHTML =
            list('Notifications', se.notifications) +
            list('Incidents', se.incidents, true) +
            list('Decorator Log', se.decorators) +
            list('Commands', cmdList);
        return;
    }

    if (state.activeTab === 'reports') {
        if (!state.reportsCache) {
            container.innerHTML = `<div class="text-slate-400 italic">Running visitors...</div>`;
            try {
                state.reportsCache = await fetch(`${API}/api/instances/${inst.id}/reports`)
                    .then((r) => r.json());
            } catch (e) {
                container.innerHTML = `<div class="text-red-600 text-sm">Failed to load.</div>`;
                return;
            }
        }
        const r = state.reportsCache;
        container.innerHTML = `
            <h3 class="font-display text-xs font-bold text-slate-900 uppercase tracking-wide mb-2">Audit Report</h3>
            <pre class="bg-slate-50 rounded-xl p-3 text-[11px] text-slate-700 overflow-x-auto whitespace-pre-wrap mb-4">${(r.audit || '').trim()}</pre>
            <h3 class="font-display text-xs font-bold text-slate-900 uppercase tracking-wide mb-2">Bottlenecks</h3>
            <pre class="bg-slate-50 rounded-xl p-3 text-[11px] text-slate-700 overflow-x-auto whitespace-pre-wrap">${(r.bottlenecks || '').trim()}</pre>
        `;
    }
}

/* 
   Undo / Cancel
   */

async function doUndo(id) {
    const ok = await showConfirm('Undo last step', 'Roll the instance back to the previous checkpoint? History is kept.');
    if (!ok) return;

    try {
        const res = await fetch(`${API}/api/instances/${id}/undo`, { method: 'POST' }).then((r) => r.json());
        if (res.ok) {
            if (!res.undone) showToast('Nothing to undo.', 'info');
            else showToast('Rolled back one step.', 'success');
            viewInstance(id);
        } else {
            showAlert('Undo failed', res.error || 'Unknown error');
        }
    } catch (e) {
        showAlert('Network error', e.message);
    }
}

async function doCancel(id) {
    const ok = await showConfirm('Cancel instance', 'Cancel this instance? Completed external calls will be reversed.');
    if (!ok) return;

    try {
        const res = await fetch(`${API}/api/instances/${id}/cancel`, { method: 'POST' }).then((r) => r.json());
        if (res.ok) { showToast('Instance cancelled.', 'success'); viewInstance(id); }
        else showAlert('Cancel failed', res.error || 'Unknown error');
    } catch (e) {
        showAlert('Network error', e.message);
    }
}

/* 
   Tree
   */

function renderTree(node) {
    const container = document.getElementById('tree-container');
    if (!container) return;
    if (!node) { container.innerHTML = `<span class="text-slate-400 italic">No tree data.</span>`; return; }

    container.innerHTML = renderTreeNode(node);

    document.querySelectorAll('.tree-item').forEach((el) => {
        el.onclick = () => {
            document.querySelectorAll('.tree-item').forEach((x) => x.classList.remove('selected'));
            el.classList.add('selected');
            const itemId = el.getAttribute('data-id');
            const itemData = findItemInTree(state.selectedInstance.tree, itemId);
            if (itemData) renderActionPanel(itemData);
        };
    });
}

function renderTreeNode(node) {
    if (node.type === 'stage') {
        return `
            <div class="mt-3 first:mt-0">
                <div class="flex items-center justify-between px-3 py-2 bg-slate-50 rounded-lg border border-slate-200 gap-2">
                    <div class="flex items-center gap-2 min-w-0">
                        <span class="w-1 h-4 rounded-full bg-accent shrink-0"></span>
                        <span class="font-semibold text-sm text-slate-800 truncate" title="${node.name}">${node.name}</span>
                    </div>
                    <span class="text-[10px] font-semibold text-slate-500 bg-slate-200/60 px-1.5 py-0.5 rounded shrink-0">${node.mode}</span>
                </div>
                <div class="tree-children mt-1">
                    ${node.children.map(renderTreeNode).join('')}
                </div>
            </div>
        `;
    }
    if (node.type === 'workitem') {
        const pill = statePill(node.state, node.isSkipped);
        const check = node.isComplete && !node.isSkipped
            ? `<span class="text-[10px] font-semibold text-emerald-600 bg-emerald-50 px-1.5 py-0.5 rounded shrink-0">done</span>`
            : '';
        return `
            <div class="tree-item flex items-start gap-2 px-3 py-2 mt-1 rounded-lg
                        cursor-pointer hover:bg-slate-50 transition-colors"
                data-id="${node.id}">
                <div class="flex items-start gap-2 flex-1 min-w-0">
                    <span class="w-1.5 h-1.5 rounded-full bg-brand shrink-0 mt-1.5"></span>
                    <span class="text-sm text-slate-700 break-words min-w-0" title="${node.name}">${node.name}</span>
                    ${check}
                </div>
                <div class="shrink-0 mt-0.5">${pill}</div>
            </div>
        `;
    }
    return '';
}

function statePill(state, isSkipped) {
    if (isSkipped) {
        return `<span class="inline-flex items-center px-2 py-0.5 rounded-full text-[10px] font-semibold bg-slate-100 text-slate-400 shrink-0">Skipped</span>`;
    }
    const map = {
        Created:    'bg-slate-100 text-slate-600',
        Available:  'bg-blue-50 text-blue-700',
        Assigned:   'bg-purple-50 text-purple-700',
        InProgress: 'bg-amber-50 text-amber-700',
        Completed:  'bg-emerald-50 text-emerald-700',
        Rejected:   'bg-red-50 text-red-700',
        Escalated:  'bg-orange-50 text-orange-700',
        Cancelled:  'bg-slate-100 text-slate-500',
    };
    const cls = map[state] || 'bg-slate-100 text-slate-600';
    return `<span class="inline-flex items-center px-2 py-0.5 rounded-full text-[10px] font-semibold ${cls} shrink-0">${state}</span>`;
}

function findItemInTree(node, id) {
    if (!node) return null;
    if (node.type === 'workitem' && node.id === id) return node;
    if (node.type === 'stage') {
        for (const child of node.children) {
            const found = findItemInTree(child, id);
            if (found) return found;
        }
    }
    return null;
}

/* 
   Action panel
   */

function renderActionPanel(item) {
    const container = document.getElementById('action-container');
    if (!container) return;

    const serviceButton = item.serviceConnector
        ? `<button class="w-full px-4 py-2.5 rounded-xl border border-slate-200 bg-white text-sm
                        font-medium text-slate-700 hover:bg-slate-50 transition-colors cursor-pointer"
                   onclick="doAction('${item.id}', 'call-service')">
               Call ${item.serviceConnector}
           </button>`
        : '';

    let buttons = '';
    if (item.state === 'Assigned') {
        buttons += `
            <button class="w-full px-4 py-2.5 rounded-xl bg-brand text-white text-sm font-semibold
                        hover:bg-brand-hover transition-colors cursor-pointer"
                    onclick="doAction('${item.id}', 'start')">
                Start Work
            </button>
            <button class="w-full px-4 py-2.5 rounded-xl border border-amber-200 bg-white text-sm
                        font-medium text-amber-700 hover:bg-amber-50 transition-colors cursor-pointer"
                    onclick="doAction('${item.id}', 'escalate')">
                Escalate
            </button>
            ${serviceButton}
        `;
    } else if (item.state === 'InProgress') {
        buttons += `
            <button class="w-full px-4 py-2.5 rounded-xl bg-brand text-white text-sm font-semibold
                        hover:bg-brand-hover transition-colors cursor-pointer"
                    onclick="doAction('${item.id}', 'complete')">
                Complete
            </button>
            <button class="w-full px-4 py-2.5 rounded-xl border border-red-200 bg-white text-sm
                        font-medium text-red-600 hover:bg-red-50 transition-colors cursor-pointer"
                    onclick="doAction('${item.id}', 'reject')">
                Reject
            </button>
            ${serviceButton}
        `;
    }

    const approvalLevel = item.data && item.data['approval.level'];
    const outcome = item.outcome;
    const reason = item.failureReason;

    container.innerHTML = `
        <div class="not-italic">
            <div class="flex items-center gap-3 mb-1 flex-wrap">
                <h3 class="font-display text-xl font-bold text-slate-900">${item.name}</h3>
                ${statePill(item.state, item.isSkipped)}
            </div>
            ${reason ? `<p class="text-sm text-red-600 mt-1">${reason}</p>` : ''}

            <div class="mt-5 space-y-2 text-sm">
                <div class="flex justify-between gap-4">
                    <span class="text-slate-500">ID</span>
                    <code class="text-slate-800 text-right break-all">${item.id}</code>
                </div>
                <div class="flex justify-between gap-4">
                    <span class="text-slate-500">Role</span>
                    <span class="text-slate-800 text-right">${item.role || '—'}</span>
                </div>
                <div class="flex justify-between gap-4">
                    <span class="text-slate-500">Assignee</span>
                    <span class="text-slate-800 text-right">${item.assignee || 'None'}</span>
                </div>
                <div class="flex justify-between gap-4">
                    <span class="text-slate-500">Rule</span>
                    <span class="text-slate-800 text-right">${item.assignmentRule || 'default'}</span>
                </div>
                ${approvalLevel ? `
                    <div class="flex justify-between gap-4">
                        <span class="text-slate-500">Approval level</span>
                        <span class="text-slate-800 text-right font-medium">${approvalLevel}</span>
                    </div>
                ` : ''}
                ${outcome ? `
                    <div class="flex justify-between gap-4">
                        <span class="text-slate-500">Outcome</span>
                        <span class="text-slate-800 text-right font-medium">${outcome}</span>
                    </div>
                ` : ''}
            </div>

            <hr class="my-5 border-slate-200">

            <details class="group">
                <summary class="cursor-pointer text-sm font-semibold text-slate-700
                               hover:text-slate-900 transition-colors select-none">
                    Advanced: override instance data
                </summary>
                <p class="text-xs text-slate-500 mt-2 mb-3 leading-relaxed">
                    Overwrite a key in the instance payload. Guards, approval routing and
                    validation decorators read these values.
                </p>
                <div class="flex gap-2">
                    <input type="text" id="data-key" placeholder="key"
                           class="flex-1 min-w-0 rounded-xl border border-slate-200 px-3 py-2 text-sm
                                  focus:outline-none focus:ring-2 focus:ring-brand/30 focus:border-brand">
                    <input type="text" id="data-val" placeholder="value"
                           class="flex-1 min-w-0 rounded-xl border border-slate-200 px-3 py-2 text-sm
                                  focus:outline-none focus:ring-2 focus:ring-brand/30 focus:border-brand">
                    <button onclick="provideData()"
                            class="px-4 py-2 rounded-xl border border-slate-200 bg-white text-sm
                                   font-medium text-slate-700 hover:bg-slate-50 transition-colors cursor-pointer shrink-0">
                        Save
                    </button>
                </div>
            </details>

            <hr class="my-5 border-slate-200">

            <div class="flex flex-col gap-2">
                ${buttons || `<p class="text-sm text-slate-400 italic">No actions available in this state.</p>`}
            </div>
        </div>
    `;
}

/* 
   Actions
   */

async function doAction(itemId, action) {
    const instId = state.selectedInstance.id;
    let body = {};

    if (action === 'complete') {
        const item = findItemInTree(state.selectedInstance.tree, itemId);
        const itemName = item ? item.name : '';
        const options = OUTCOME_OPTIONS[itemName] || ['done'];

        // If only one option, ask for confirmation directly.
        if (options.length === 1) {
            const ok = await showConfirm('Complete work item', `Complete "${itemName}" with outcome "${options[0]}"?`);
            if (!ok) return;
            body.outcome = options[0];
        } else {
            const outcome = await showOutcomeButtonModal(itemName, options);
            if (outcome === null) return;
            body.outcome = outcome;
        }
    } else if (action === 'reject' || action === 'escalate') {
        const reason = await showReasonInput(action, action === 'reject' ? 'Not approved' : 'No response in time');
        if (reason === null) return;
        body.reason = reason;
    }

    try {
        const res = await fetch(`${API}/api/instances/${instId}/items/${encodeURIComponent(itemId)}/${action}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body),
        }).then((r) => r.json());

        if (res.ok) {
            viewInstance(instId);
        } else {
            showAlert('Engine refused', res.error || 'The engine refused this action.');
        }
    } catch (e) {
        showAlert('Network error', e.message);
    }
}

/** Modal that shows outcome buttons and resolves with the chosen outcome. */
function showOutcomeButtonModal(itemName, options) {
    return new Promise((resolve) => {
        const root = document.getElementById('modal-root');
        root.innerHTML = `
            <div class="fixed inset-0 z-[100] flex items-center justify-center p-4
                        bg-slate-900/40 backdrop-blur-sm" id="modal-backdrop">
                <div class="modal-enter bg-white rounded-2xl shadow-xl border border-slate-200
                            w-full max-w-sm p-6">
                    <h2 class="font-display text-lg font-bold text-slate-900 mb-1">Complete</h2>
                    <p class="text-sm text-slate-500 mb-4">
                        Select the outcome for <strong class="text-slate-800">${itemName}</strong>.
                    </p>
                    <div class="flex flex-col gap-2">
                        ${options.map((opt) => `
                            <button type="button"
                                    class="outcome-choice w-full text-left px-4 py-3 rounded-xl border-2
                                           border-slate-200 bg-white hover:border-brand hover:bg-brand-light
                                           transition-colors cursor-pointer font-semibold text-slate-800"
                                    data-outcome="${opt}">
                                ${opt}
                            </button>
                        `).join('')}
                    </div>
                    <div class="flex justify-end mt-4">
                        <button type="button" id="outcome-cancel"
                                class="px-4 py-2 rounded-lg text-sm font-semibold text-slate-700
                                       bg-white border border-slate-200 hover:bg-slate-50 transition-colors cursor-pointer">
                            Cancel
                        </button>
                    </div>
                </div>
            </div>
        `;

        root.querySelectorAll('.outcome-choice').forEach((btn) => {
            btn.onclick = () => {
                const val = btn.dataset.outcome;
                closeModal();
                resolve(val);
            };
        });
        document.getElementById('outcome-cancel').onclick = () => { closeModal(); resolve(null); };
        document.getElementById('modal-backdrop').addEventListener('click', (e) => {
            if (e.target.id === 'modal-backdrop') { closeModal(); resolve(null); }
        });
    });
}

async function provideData() {
    const key = document.getElementById('data-key').value.trim();
    const val = document.getElementById('data-val').value;
    if (!key) return;
    if (key.charAt(0) === '_') {
        showToast('Keys starting with "_" are reserved.', 'error');
        return;
    }

    const instId = state.selectedInstance.id;
    try {
        const res = await fetch(`${API}/api/instances/${instId}/data`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ [key]: val }),
        }).then((r) => r.json());

        if (res.ok) {
            showToast(`Saved ${key}`, 'success');
            viewInstance(instId);
        } else {
            showAlert('Error', res.error || 'Failed to save data');
        }
    } catch (e) {
        showAlert('Network error', e.message);
    }
}

/* 
   Start Workflow modal
   */

document.getElementById('btn-new').onclick = async () => {
    if (!state.definitions || state.definitions.length === 0) {
        return showAlert('No definitions', 'No workflow definitions are loaded. Try refreshing the page.');
    }

    state.currentCases = [];
    state.selectedCaseIndex = -1;

    // Build the scenario selector
    const scenarioOptions = state.definitions.map((d) =>
        `<option value="${d.id}">${d.title}</option>`).join('');

    showModal({
        title: 'Start Workflow',
        width: 'max-w-lg',
        bodyHTML: `
            <div class="space-y-4">
                <div>
                    <label class="block text-xs font-semibold text-slate-500 uppercase tracking-wide mb-1.5">Scenario</label>
                    <select id="modal-scenario-select"
                            class="w-full rounded-xl border border-slate-200 px-3 py-2.5 text-sm bg-white
                                   focus:outline-none focus:ring-2 focus:ring-brand/30 focus:border-brand">
                        ${scenarioOptions}
                    </select>
                </div>

                <div>
                    <label class="block text-xs font-semibold text-slate-500 uppercase tracking-wide mb-1.5">Sample case</label>
                    <div id="modal-cases" class="space-y-2 max-h-48 overflow-y-auto"></div>
                </div>

                <div id="modal-data-fields" class="space-y-3"></div>
            </div>
        `,
        actions: [{ label: 'Cancel', value: null, style: 'secondary' }],
    }).then(() => { /* handled below */ });

    // Wire up the scenario selector and load cases
    const sel = document.getElementById('modal-scenario-select');
    sel.onchange = () => loadCasesForCurrentScenario();
    await loadCasesForCurrentScenario();

    // Override the action buttons: replace Cancel-only with Cancel + Start
    const actionsDiv = document.getElementById('modal-actions');
    actionsDiv.innerHTML = '';
    const cancelBtn = document.createElement('button');
    cancelBtn.type = 'button';
    cancelBtn.textContent = 'Cancel';
    cancelBtn.className = 'px-4 py-2 rounded-lg text-sm font-semibold text-slate-700 bg-white border border-slate-200 hover:bg-slate-50 transition-colors cursor-pointer';
    cancelBtn.onclick = () => closeModal();

    const startBtn = document.createElement('button');
    startBtn.type = 'button';
    startBtn.textContent = 'Start Workflow';
    startBtn.className = 'px-4 py-2 rounded-lg text-sm font-semibold text-white bg-brand hover:bg-brand-hover transition-colors cursor-pointer';
    startBtn.onclick = () => submitWorkflow();

    actionsDiv.appendChild(cancelBtn);
    actionsDiv.appendChild(startBtn);
};

async function loadCasesForCurrentScenario() {
    const sel = document.getElementById('modal-scenario-select');
    if (!sel) return;
    const scenarioId = sel.value;
    state.currentCases = [];
    state.selectedCaseIndex = -1;

    try {
        const r = await fetch(`${API}/api/definitions/${encodeURIComponent(scenarioId)}/cases`)
            .then((x) => x.json());
        if (r.ok && Array.isArray(r.cases)) state.currentCases = r.cases;
    } catch (e) { /* fall through */ }

    if (state.currentCases.length > 0) state.selectedCaseIndex = 0;
    renderCaseCards();
    renderModalFields();
}

function renderCaseCards() {
    const container = document.getElementById('modal-cases');
    if (!container) return;

    if (state.currentCases.length === 0) {
        container.innerHTML = `<p class="text-xs text-slate-400 italic">No sample cases. Enter data manually below.</p>`;
        return;
    }

    container.innerHTML = state.currentCases.map((c, i) => `
        <button type="button"
                class="case-card w-full text-left px-3 py-2.5 rounded-xl border-2 transition-colors cursor-pointer
                       ${state.selectedCaseIndex === i ? 'border-brand bg-brand-light' : 'border-slate-200 bg-white hover:border-slate-300'}"
                onclick="selectCaseByIndex(${i})">
            <div class="font-semibold text-sm text-slate-800">${c.label}</div>
            <div class="text-xs text-slate-500 mt-0.5 leading-relaxed">${c.story}</div>
        </button>
    `).join('') + `
        <button type="button"
                class="case-card w-full text-left px-3 py-2.5 rounded-xl border-2 transition-colors cursor-pointer
                       ${state.selectedCaseIndex === -1 ? 'border-brand bg-brand-light' : 'border-slate-200 bg-white hover:border-slate-300'}"
                onclick="selectCaseByIndex(-1)">
            <div class="font-semibold text-sm text-slate-800">Custom</div>
            <div class="text-xs text-slate-500 mt-0.5">Enter your own values in the fields below.</div>
        </button>
    `;
}

function selectCaseByIndex(index) {
    state.selectedCaseIndex = index;
    renderCaseCards();
    renderModalFields();
}

function caseDataForCurrentSelection() {
    if (typeof state.selectedCaseIndex !== 'number') return {};
    if (state.selectedCaseIndex < 0) return {};
    if (!state.currentCases || state.selectedCaseIndex >= state.currentCases.length) return {};
    return state.currentCases[state.selectedCaseIndex].data || {};
}

function renderModalFields() {
    const container = document.getElementById('modal-data-fields');
    if (!container) return;

    const scenarioId = document.getElementById('modal-scenario-select').value;
    const caseData = caseDataForCurrentSelection();
    const v = (k, def) => (caseData[k] !== undefined ? caseData[k] : def);

    const field = (label, id, type, value) => `
        <div>
            <label class="block text-xs font-semibold text-slate-500 uppercase tracking-wide mb-1">${label}</label>
            <input type="${type}" id="f-${id}" value="${value}"
                   class="w-full rounded-xl border border-slate-200 px-3 py-2 text-sm
                          focus:outline-none focus:ring-2 focus:ring-brand/30 focus:border-brand">
        </div>
    `;

    if (scenarioId === 'motor-claim') {
        container.innerHTML = `
            ${field('Claimant Name', 'claimant', 'text', v('claimant', 'Thandi Mokoena'))}
            ${field('Policy Number', 'policyNumber', 'text', v('policyNumber', 'POL-1001'))}
            ${field('Claim Amount', 'claimAmount', 'number', v('claimAmount', '18500'))}
            ${field('Police Case Number', 'policeCaseNumber', 'text', v('policeCaseNumber', 'CAS-114/10/2026'))}
            ${field('Bank Account', 'bankAccount', 'text', v('bankAccount', '62012345678'))}
            ${field('Fraud Score (0-100)', 'fraudScore', 'number', v('fraudScore', '12'))}
        `;
    } else if (scenarioId === 'bursary-application') {
        container.innerHTML = `
            ${field('Student Number', 'studentNumber', 'text', v('studentNumber', 'u25123456'))}
            ${field('Bursary Type (merit / need)', 'bursaryType', 'text', v('bursaryType', 'merit'))}
            ${field('Requested Amount', 'requestedAmount', 'number', v('requestedAmount', '8000'))}
            ${field('Fund Balance', 'fundBalance', 'number', v('fundBalance', '900000'))}
            ${field('Bank Account', 'bankAccount', 'text', v('bankAccount', '10099887766'))}
        `;
    } else {
        container.innerHTML = `<p class="text-sm text-slate-400 italic">Unknown scenario.</p>`;
        return;
    }

    // Show extra case data that the form doesn't expose
    const known = new Set(
        Array.from(container.querySelectorAll('input'))
            .map((i) => i.id.replace(/^f-/, ''))
    );
    const extras = Object.keys(caseData).filter((k) => !known.has(k));
    if (extras.length > 0) {
        const extrasDiv = document.createElement('div');
        extrasDiv.className = 'border-t border-dashed border-slate-200 pt-3 mt-1';
        extrasDiv.innerHTML = `
            <p class="text-[11px] text-slate-400 mb-1">Additional case data (submitted automatically):</p>
            ${extras.map((k) => `
                <div class="flex justify-between text-xs py-0.5 gap-2">
                    <span class="text-slate-500">${k}</span>
                    <code class="text-slate-700 break-all text-right">${caseData[k]}</code>
                </div>
            `).join('')}
        `;
        container.appendChild(extrasDiv);
    }
}

async function submitWorkflow() {
    const scenarioId = document.getElementById('modal-scenario-select').value;
    const caseData = { ...caseDataForCurrentSelection() };
    const formData = {};

    document.querySelectorAll('#modal-data-fields input').forEach((inp) => {
        formData[inp.id.replace(/^f-/, '')] = inp.value;
    });

    const data = { ...caseData, ...formData };

    try {
        const res = await fetch(`${API}/api/instances`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ scenarioId, data }),
        }).then((r) => r.json());

        if (res.ok) {
            closeModal();
            showToast('Workflow started.', 'success');
            init();
        } else {
            showAlert('Could not start', res.error || 'Unknown error');
        }
    } catch (e) {
        showAlert('Network error', e.message);
    }
}

/* 
   Start
   */

init();