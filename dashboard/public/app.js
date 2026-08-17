import { initializeApp } from "https://www.gstatic.com/firebasejs/10.8.1/firebase-app.js";
import { getDatabase, ref, onValue, set, query, limitToLast } from "https://www.gstatic.com/firebasejs/10.8.1/firebase-database.js";
import { getAuth, signInWithEmailAndPassword, onAuthStateChanged, signOut } from "https://www.gstatic.com/firebasejs/10.8.1/firebase-auth.js";

// Configuration Firebase
const firebaseConfig = {
    apiKey: "AIzaSyA1XJf_CcvcEL3vL_-HmO9MoeUK6qQi9ss",
    authDomain: "automatisationpompe.firebaseapp.com",
    databaseURL: "https://automatisationpompe-default-rtdb.europe-west1.firebasedatabase.app/",
    projectId: "automatisationpompe",
    storageBucket: "automatisationpompe.firebasestorage.app",
    messagingSenderId: "42829684142",
    appId: "1:42829684142:web:cd3c18b02c863f90edfc3f",
    measurementId: "G-W3HPTJ997H"
};

const app = initializeApp(firebaseConfig);
const database = getDatabase(app);
const auth = getAuth(app);

// DOM Auth
const authContainer = document.getElementById('auth-container');
const dashboardContainer = document.getElementById('dashboard-container');
const loginBtn = document.getElementById('login-btn');
const authError = document.getElementById('auth-error');
const logoutBtn = document.getElementById('logout-btn');

// DOM Dashboard
const connBadge = document.getElementById('conn-status');
const connText = document.getElementById('conn-text');
const faultBanner = document.getElementById('fault-banner');
const faultText = document.getElementById('fault-text');

// DOM Citerne
const waterLevel = document.getElementById('water-level');
const sensorHighVisual = document.getElementById('sensor-high-visual');
const sensorLowVisual = document.getElementById('sensor-low-visual');
const levelText = document.getElementById('level-text');
const dotHigh = document.getElementById('dot-high');
const labelHigh = document.getElementById('label-high');
const dotLow = document.getElementById('dot-low');
const labelLow = document.getElementById('label-low');

// DOM Navigation & Journal
const tabDashboard = document.getElementById('tab-dashboard');
const tabJournal = document.getElementById('tab-journal');
const dashboardView = document.querySelector('.dashboard-grid');
const journalView = document.getElementById('journal-view');
const filtersContainer = document.getElementById('filters-container');
const logsContainer = document.getElementById('logs-container');

// DOM Pompe & Commandes
const pumpGlow = document.getElementById('pump-glow');
const stateDisplay = document.getElementById('state-display');
const modeDisplayText = document.getElementById('mode-display-text');

const btnModeAuto = document.getElementById('btn-mode-auto');
const btnModeManual = document.getElementById('btn-mode-manual');
const modeHint = document.getElementById('mode-hint');

const btnStart = document.getElementById('btn-start');
const btnStop = document.getElementById('btn-stop');
const controlHint = document.getElementById('control-hint');

const btnModeMaint = document.getElementById('btn-mode-maint');

// Local State
let currentMode = "MANUAL";
let currentState = "OFF";
let currentHigh = false;
let currentLow = false;
let allLogs = [];
let currentFilter = "Tous";

// --- AUTHENTIFICATION ---
loginBtn.addEventListener('click', () => {
    const email = document.getElementById('email').value;
    const password = document.getElementById('password').value;
    
    authError.style.display = 'none';
    loginBtn.innerText = "CONNEXION...";
    
    signInWithEmailAndPassword(auth, email, password)
        .then(() => {
            loginBtn.innerText = "SE CONNECTER";
        })
        .catch((error) => {
            loginBtn.innerText = "SE CONNECTER";
            authError.style.display = 'block';
            if(error.code === 'auth/invalid-credential') {
                authError.innerText = "Email ou mot de passe incorrect.";
            } else {
                authError.innerText = error.message;
            }
        });
});

logoutBtn.addEventListener('click', () => {
    signOut(auth);
});

onAuthStateChanged(auth, (user) => {
    if (user) {
        authContainer.style.display = 'none';
        dashboardContainer.style.display = 'flex';
        initDashboardListeners();
    } else {
        authContainer.style.display = 'flex';
        dashboardContainer.style.display = 'none';
    }
});


// --- LOGIQUE DASHBOARD ---
let listenersAttached = false;
let lastSeenTs = 0;

function updateWaterLevelVisual() {
    // Determine level string and class
    if (currentHigh) {
        levelText.innerText = "PLEINE";
        waterLevel.className = "water level-full";
    } else if (!currentLow) {
        levelText.innerText = "VIDE";
        waterLevel.className = "water level-empty";
    } else if (currentState === "ON") {
        levelText.innerText = "EN REMPLISSAGE";
        waterLevel.className = "water level-mid";
    } else {
        levelText.innerText = "NORMAL";
        waterLevel.className = "water level-mid";
    }
}

function initDashboardListeners() {
    if(listenersAttached) return;
    listenersAttached = true;

    // Liveness / Présence de l'ESP32
    setInterval(() => {
        const now = Math.floor(Date.now() / 1000); // en secondes
        if (lastSeenTs > 0 && (now - lastSeenTs) <= 20) {
            // L'ESP32 a pingé il y a moins de 20 secondes
            connBadge.classList.remove('offline');
            connText.innerText = 'En ligne';
        } else {
            connBadge.classList.add('offline');
            connText.innerText = 'Hors ligne';
        }
    }, 2000);

    onValue(ref(database, 'pump/last_seen'), (snapshot) => {
        lastSeenTs = snapshot.val() || 0;
    });

    // Niveau Bas
    onValue(ref(database, 'pump/level_low'), (snapshot) => {
        currentLow = snapshot.val();
        if(currentLow) {
            sensorLowVisual.classList.add('active');
            dotLow.classList.add('active');
            labelLow.innerText = 'immergé';
        } else {
            sensorLowVisual.classList.remove('active');
            dotLow.classList.remove('active');
            labelLow.innerText = 'sec';
        }
        updateWaterLevelVisual();
    });

    // Niveau Haut
    onValue(ref(database, 'pump/level_high'), (snapshot) => {
        currentHigh = snapshot.val();
        if(currentHigh) {
            sensorHighVisual.classList.add('active');
            dotHigh.classList.add('active');
            labelHigh.innerText = 'immergé';
        } else {
            sensorHighVisual.classList.remove('active');
            dotHigh.classList.remove('active');
            labelHigh.innerText = 'sec';
        }
        updateWaterLevelVisual();
    });

    // Défauts
    onValue(ref(database, 'pump/fault'), (snapshot) => {
        const val = snapshot.val();
        if(val && val !== "NONE") {
            faultBanner.style.display = 'flex';
            faultText.innerText = val;
        } else {
            faultBanner.style.display = 'none';
        }
    });

    // Mode
    onValue(ref(database, 'pump/mode'), (snapshot) => {
        currentMode = snapshot.val();
        if (currentMode === "AUTO") {
            modeDisplayText.innerText = "Automatique";
            btnModeAuto.classList.add('active');
            btnModeManual.classList.remove('active');
            btnModeMaint.classList.remove('active');
            modeHint.innerText = "La pompe gère automatiquement le niveau selon les capteurs.";
            
            // Disable manual controls
            btnStart.disabled = true;
            btnStop.disabled = true;
            controlHint.innerText = "Passez en mode Manuel pour commander la pompe directement.";
            
        } else if (currentMode === "MANUAL") {
            modeDisplayText.innerText = "Manuel";
            btnModeManual.classList.add('active');
            btnModeAuto.classList.remove('active');
            btnModeMaint.classList.remove('active');
            modeHint.innerText = "Démarrage et arrêt manuels depuis ce tableau de bord.";
            
            // Enable manual controls
            btnStart.disabled = false;
            btnStop.disabled = false;
            controlHint.innerText = "Commandes manuelles actives.";
            
        } else if (currentMode === "MAINTENANCE") {
            modeDisplayText.innerText = "Entretien";
            btnModeMaint.classList.add('active');
            btnModeManual.classList.remove('active');
            btnModeAuto.classList.remove('active');
            modeHint.innerText = "Pompe verrouillée pour nettoyage.";
            
            // Disable manual controls
            btnStart.disabled = true;
            btnStop.disabled = true;
            controlHint.innerText = "La pompe est coupée pendant l'entretien.";
        }
    });

    // État Relais
    onValue(ref(database, 'pump/relay_state'), (snapshot) => {
        currentState = snapshot.val();
        if (currentState === "ON") {
            stateDisplay.innerText = "En marche";
            stateDisplay.classList.add('running');
            pumpGlow.classList.add('running');
            btnStart.disabled = true;
            btnStart.classList.add('active-cmd');
            btnStop.classList.remove('active-cmd');
            if(currentMode === "MANUAL") btnStop.disabled = false;
        } else {
            stateDisplay.innerText = "À l'arrêt";
            stateDisplay.classList.remove('running');
            pumpGlow.classList.remove('running');
            btnStop.disabled = true;
            btnStop.classList.add('active-cmd');
            btnStart.classList.remove('active-cmd');
            if(currentMode === "MANUAL") btnStart.disabled = false;
        }
        updateWaterLevelVisual(); // update water animation if filling
    });

    // Actions Boutons
    btnModeAuto.addEventListener('click', () => {
        set(ref(database, 'pump/set_mode'), "AUTO");
    });

    btnModeManual.addEventListener('click', () => {
        set(ref(database, 'pump/set_mode'), "MANUAL");
    });

    btnModeMaint.addEventListener('click', () => {
        set(ref(database, 'pump/set_mode'), "MAINTENANCE");
    });

    btnStart.addEventListener('click', () => {
        if (currentMode !== "AUTO") {
            set(ref(database, 'pump/command_state'), "ON");
        }
    });

    btnStop.addEventListener('click', () => {
        if (currentMode !== "AUTO" && currentMode !== "MAINTENANCE") {
            set(ref(database, 'pump/command_state'), "OFF");
        }
    });



    // Journal Logic
    const logsQuery = query(ref(database, 'pump/logs'), limitToLast(50));
    onValue(logsQuery, (snapshot) => {
        allLogs = [];
        snapshot.forEach((childSnap) => {
            allLogs.push(childSnap.val());
        });
        allLogs.reverse(); // Newest first
        renderLogs();
    });
}

// Navigation
tabDashboard.addEventListener('click', () => {
    tabDashboard.classList.add('active');
    tabJournal.classList.remove('active');
    dashboardView.style.display = 'grid';
    journalView.style.display = 'none';
});

tabJournal.addEventListener('click', () => {
    tabJournal.classList.add('active');
    tabDashboard.classList.remove('active');
    dashboardView.style.display = 'none';
    journalView.style.display = 'flex';
    journalView.style.flexDirection = 'column';
    journalView.style.gap = '20px';
});

// Filters
if (filtersContainer) {
    filtersContainer.addEventListener('click', (e) => {
        if (e.target.classList.contains('btn-filter')) {
            document.querySelectorAll('.btn-filter').forEach(btn => btn.classList.remove('active'));
            e.target.classList.add('active');
            currentFilter = e.target.getAttribute('data-filter');
            renderLogs();
        }
    });
}

function renderLogs() {
    logsContainer.innerHTML = '';
    const filtered = allLogs.filter(log => currentFilter === 'Tous' || log.category === currentFilter);
    
    if (filtered.length === 0) {
        logsContainer.innerHTML = '<div class="empty-log">Aucun événement pour ce filtre.</div>';
        return;
    }

    filtered.forEach(log => {
        const div = document.createElement('div');
        div.className = 'log-entry';
        div.innerHTML = `
            <div class="log-ts">${log.ts || '--/-- --:--'}</div>
            <div class="log-category">
                <span class="dot sev-${log.sev || 'info'}"></span>
                <span>${log.category || 'Inconnu'}</span>
            </div>
            <div class="log-msg">${log.message || ''}</div>
        `;
        logsContainer.appendChild(div);
    });
}
