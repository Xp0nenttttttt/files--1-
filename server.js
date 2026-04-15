// ─────────────────────────────────────────────────────────────────
//  Backend Player Titles — Node.js + Express
//  npm install express cors jsonwebtoken bcryptjs
// ─────────────────────────────────────────────────────────────────
const express = require("express");
const cors = require("cors");
const jwt = require("jsonwebtoken");
const bcrypt = require("bcryptjs");
const fs = require("fs");
const path = require("path");

const app = express();
const PORT = process.env.PORT || 3000;

// ── Config ────────────────────────────────────────────────────────
const SECRET = process.env.JWT_SECRET || "change_this_secret_key";
const DATA_FILE = path.join(__dirname, "data", "titles_data.json");

// Mot de passe admin hashé (généré une seule fois via bcrypt.hashSync)
// Par défaut : "admin1234" → remplace par ton propre hash !
const ADMIN_PASS_HASH = process.env.ADMIN_HASH ||
    bcrypt.hashSync("admin1234", 10);

app.use(cors());
app.use(express.json());
app.use(express.static('.'));
app.use((req, res, next) => {
    res.set('Content-Security-Policy', "default-src 'self'; style-src 'self' 'unsafe-inline'; script-src 'self' 'unsafe-inline'; font-src 'self' https://fonts.googleapis.com; connect-src 'self'");
    next();
});

// ── Helpers ───────────────────────────────────────────────────────
function readData() {
    try {
        return JSON.parse(fs.readFileSync(DATA_FILE, "utf-8"));
    } catch {
        return { titles: [], teams: [], admins: [] };
    }
}

function writeData(data) {
    fs.mkdirSync(path.dirname(DATA_FILE), { recursive: true });
    fs.writeFileSync(DATA_FILE, JSON.stringify(data, null, 2), "utf-8");
}

function authMiddleware(req, res, next) {
    const auth = req.headers.authorization || "";
    const token = auth.startsWith("Bearer ") ? auth.slice(7) : null;
    if (!token) return res.status(401).json({ error: "Token requis" });
    try {
        req.admin = jwt.verify(token, SECRET);
        next();
    } catch {
        res.status(403).json({ error: "Token invalide ou expiré" });
    }
}

// ════════════════════════════════════════════════════════════════
//  ROUTES PUBLIQUES
// ════════════════════════════════════════════════════════════════

// GET /api/data  ← Le mod Geode fetch cette route au démarrage
app.get("/api/data", (req, res) => {
    res.json(readData());
});

// POST /api/admin/login  ← Authentification admin
app.post("/api/admin/login", (req, res) => {
    const { password } = req.body;
    if (!password || !bcrypt.compareSync(password, ADMIN_PASS_HASH)) {
        return res.status(401).json({ error: "Mot de passe incorrect" });
    }
    const token = jwt.sign({ role: "admin" }, SECRET, { expiresIn: "8h" });
    res.json({ token, expiresIn: "8h" });
});

// ════════════════════════════════════════════════════════════════
//  ROUTES ADMIN PROTÉGÉES
// ════════════════════════════════════════════════════════════════

// POST /api/admin/titles  ← Ajoute ou met à jour un titre joueur
app.post("/api/admin/titles", authMiddleware, (req, res) => {
    const { accountId, title, color, team } = req.body;
    if (!accountId || !title)
        return res.status(400).json({ error: "accountId et title requis" });

    const data = readData();
    const idx = data.titles.findIndex(t => t.accountId === Number(accountId));

    const entry = {
        accountId: Number(accountId),
        title,
        color: color || "FFFFFF",
        team: team || ""
    };

    if (idx >= 0) data.titles[idx] = entry;      // mise à jour
    else data.titles.push(entry);         // ajout

    writeData(data);
    res.json({ success: true, entry });
});

// DELETE /api/admin/titles/:accountId  ← Supprime un titre
app.delete("/api/admin/titles/:accountId", authMiddleware, (req, res) => {
    const id = Number(req.params.accountId);
    const data = readData();
    const before = data.titles.length;
    data.titles = data.titles.filter(t => t.accountId !== id);
    if (data.titles.length === before)
        return res.status(404).json({ error: "Titre non trouvé" });
    writeData(data);
    res.json({ success: true });
});

// POST /api/admin/teams  ← Ajoute ou met à jour une équipe
app.post("/api/admin/teams", authMiddleware, (req, res) => {
    const { name, points, wins, losses, color } = req.body;
    if (!name) return res.status(400).json({ error: "name requis" });

    const data = readData();
    const idx = data.teams.findIndex(t => t.name === name);

    const entry = {
        name,
        points: Number(points) || 0,
        wins: Number(wins) || 0,
        losses: Number(losses) || 0,
        color: color || "FFFFFF"
    };

    if (idx >= 0) data.teams[idx] = entry;
    else data.teams.push(entry);

    // Recalcule les rangs par ordre de points décroissant
    data.teams.sort((a, b) => b.points - a.points);
    data.teams.forEach((t, i) => t.rank = i + 1);

    writeData(data);
    res.json({ success: true, entry });
});

// DELETE /api/admin/teams/:name  ← Supprime une équipe
app.delete("/api/admin/teams/:name", authMiddleware, (req, res) => {
    const name = decodeURIComponent(req.params.name);
    const data = readData();
    data.teams = data.teams.filter(t => t.name !== name);
    data.teams.forEach((t, i) => t.rank = i + 1);
    writeData(data);
    res.json({ success: true });
});

// POST /api/admin/admins  ← Ajoute un compte admin GD
app.post("/api/admin/admins", authMiddleware, (req, res) => {
    const { accountId } = req.body;
    if (!accountId) return res.status(400).json({ error: "accountId requis" });
    const data = readData();
    if (!data.admins.includes(Number(accountId)))
        data.admins.push(Number(accountId));
    writeData(data);
    res.json({ success: true, admins: data.admins });
});

// DELETE /api/admin/admins/:accountId
app.delete("/api/admin/admins/:accountId", authMiddleware, (req, res) => {
    const id = Number(req.params.accountId);
    const data = readData();
    data.admins = data.admins.filter(a => a !== id);
    writeData(data);
    res.json({ success: true, admins: data.admins });
});

// GET /api/admin/data  ← Récupère toutes les données (pour le panel web)
app.get("/api/admin/data", authMiddleware, (req, res) => {
    res.json(readData());
});

// ── Démarrage ──────────────────────────────────────────────────────
app.listen(PORT, () => {
    console.log(`✅ Serveur Player Titles démarré sur http://localhost:${PORT}`);
    console.log(`📁 Données : ${DATA_FILE}`);
    console.log(`🔑 Mot de passe admin par défaut : admin1234  (CHANGE-LE !)`);
});
