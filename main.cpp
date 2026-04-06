#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSpinBox>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>
#include <QScreen>
#include <QFontDatabase>
#include <QCloseEvent>  // <--- এই লাইনটি মিসিং ছিল!

// Windows API
#include <windows.h>
#include <tlhelp32.h>

// ==========================================
// DATA STRUCTURES (Pure Qt)
// ==========================================
QStringList blockedApps, blockedWebs, allowedApps, allowedWebs;

QStringList systemApps = {
    "explorer.exe", "svchost.exe", "taskmgr.exe", "cmd.exe", "conhost.exe", "csrss.exe", "dwm.exe", 
    "lsass.exe", "services.exe", "smss.exe", "wininit.exe", "winlogon.exe", "spoolsv.exe", 
    "fontdrvhost.exe", "searchui.exe", "searchindexer.exe", "sihost.exe", "taskhostw.exe", 
    "ctfmon.exe", "applicationframehost.exe", "system", "registry", "audiodg.exe",
    "searchapp.exe", "startmenuexperiencehost.exe", "shellexperiencehost.exe", "textinputhost.exe",
    "chrome.exe", "msedge.exe", "firefox.exe", "brave.exe", "opera.exe"
};

QStringList hiddenUIProcesses = {
    "svchost.exe", "smss.exe", "csrss.exe", "services.exe", "lsass.exe", "wininit.exe", 
    "winlogon.exe", "fontdrvhost.exe", "dwm.exe", "spoolsv.exe", "taskhostw.exe", 
    "conhost.exe", "system", "registry", "dllhost.exe", "wmiprvse.exe", "taskmgr.exe"
};

QStringList explicitKeywords = {
    "porn", "xxx", "sex", "nude", "nsfw", "adult video", "xvideos", "pornhub", 
    "xnxx", "xhamster", "brazzers", "onlyfans", "playboy", "naughty america", 
    "mia khalifa", "dani daniels", "johnny sins", "hot desi", "bhabi", "chudai", 
    "bangla choti", "magi", "sexy"
};

QStringList defaultAdultWebs = {
    "pornhub", "xvideos", "xnxx", "xhamster", "brazzers", "redtube", 
    "youporn", "tube8", "spankbang", "eporner", "beeg", "xmovies8"
};

QStringList defaultSystemBlocks = {"regedit.exe", "mmc.exe", "systemsettings.exe", "control.exe", "gpedit.msc"};
QStringList defaultSystemTitles = {"control panel", "registry editor", "local group policy editor", "settings"};

QStringList islamicQuotes = {
    "\"মুমিনদের বলুন, তারা যেন তাদের দৃষ্টি নত রাখে এবং তাদের যৌনাঙ্গর হেফাযত করে।\" - (সূরা আন-নূর: ৩০)",
    "\"লজ্জাশীলতা কল্যাণ ছাড়া আর কিছুই বয়ে আনে না।\" - (সহীহ বুখারী)",
    "\"আর তোমরা ব্যভিচারের কাছেও যেয়ো না। নিশ্চয়ই এটা অশ্লীল কাজ এবং মন্দ পথ।\" - (সূরা আল-ইসরা: ৩২)",
    "\"প্রতিটি ধর্মের একটি নিজস্ব স্বভাব রয়েছে, আর ইসলামের স্বভাব হলো লজ্জাশীলতা।\" - (ইবনে মাজাহ)",
    "\"আল্লাহ সবকিছু দেখছেন। নিজের চোখ ও মনকে পবিত্র রাখো।\"",
    "\"যে ব্যক্তি তার দুই চোয়ালের মধ্যবর্তী স্থান (জিহ্বা) এবং দুই পায়ের মধ্যবর্তী স্থানের জামানত আমাকে দেবে, আমি তার জান্নাতের জামানত দেব।\"",
    "\"চোখের ব্যভিচার হলো (হারাম কিছুর দিকে) তাকানো।\" - (সহীহ মুসলিম)",
    "\"ঈমান ও লজ্জাশীলতা একে অপরের সাথে যুক্ত। একটি উঠে গেলে অপরটিও উঠে যায়।\""
};

QStringList timeQuotes = {
    "\"যারা সময়কে মূল্যায়ন করে না, সময়ও তাদেরকে মূল্যায়ন করে না।\" - এ.পি.জে. আবদুল কালাম",
    "\"আজকের দিনটি নষ্ট করার অর্থ হলো ভবিষ্যতের একটি উজ্জ্বল দিন চুরি করা। কাজে ফিরে যাও!\"",
    "\"বড় কিছু অর্জন করতে হলে ছোট ছোট বিভ্রান্তিগুলোকে 'না' বলতে শিখতে হবে।\"",
    "\"সময় ও স্রোত কারও জন্য অপেক্ষা করে না। তোমার লক্ষ্যকে প্রাধান্য দাও।\"",
    "\"সাফল্যের মূল রহস্য হলো মনোযোগ ধরে রাখা। বিক্ষিপ্ত মন নিয়ে বড় কিছু জয় করা যায় না।\""
};

bool isSessionActive = false;
bool isTimeMode = false;
bool isPassMode = false;
bool useAllowMode = false; 
bool isOverlayVisible = false;

QString currentSessionPass = "";
int focusTimeTotalSeconds = 0;
int timerTicks = 0;

// ==========================================
// UTILITY FUNCTIONS (Win32 explicit W-API)
// ==========================================
bool IsRunAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

QString CleanURL(QString s) {
    s = s.toLower();
    QStringList to_erase = {"https://", "http://", "www.", "/*"};
    for (const QString& prefix : to_erase) {
        s.replace(prefix, "");
    }
    if(s.endsWith("/")) s.chop(1);
    
    QStringList domains = {".com", ".org", ".net", ".io", ".co", ".bd"};
    for (const QString& dom : domains) {
        int pos = s.indexOf(dom);
        if (pos != -1) s = s.left(pos);
    }
    return s; 
}

void CloseActiveTabAndMinimize(HWND hBrowser) {
    keybd_event(VK_CONTROL, 0, 0, 0); keybd_event('W', 0, 0, 0);              
    keybd_event('W', 0, KEYEVENTF_KEYUP, 0); keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0); 
    Sleep(50); 
    ShowWindow(hBrowser, SW_MINIMIZE);
}

QStringList GetRunningProcesses() {
    QStringList processes;
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe = {sizeof(pe)}; 
    if (Process32FirstW(h, &pe)) {
        do { processes.append(QString::fromWCharArray(pe.szExeFile)); } while (Process32NextW(h, &pe));
    }
    CloseHandle(h);
    processes.removeDuplicates();
    processes.sort(Qt::CaseInsensitive);
    return processes;
}

void LoadDataList(const QString& filename, QStringList& vec, QListWidget* listWidget) {
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                vec.append(line);
                listWidget->addItem(line);
            }
        }
        file.close();
    }
}

void SaveDataList(const QString& filename, const QStringList& vec) {
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString& item : vec) out << item << "\n";
        file.close();
    }
}

void ShowAllowedWebsitesPage() {
    static DWORD lastTime = 0; 
    if (GetTickCount() - lastTime < 3000) return; 
    lastTime = GetTickCount();

    QString htmlPath = QDir::currentPath() + "/allowed_sites.html";
    QFile file(htmlPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream html(&file);
        html << "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Allowed Websites</title>";
        html << "<style>body{font-family: 'Segoe UI', sans-serif; background-color: #f4f6f9; color: #212529; text-align: center; padding-top: 80px;}";
        html << "a {background: #007bff; color: white; text-decoration: none; font-size: 18px; padding: 12px 25px; margin: 10px; display: inline-block; border-radius: 50px; transition: all 0.3s;} a:hover {background: #0056b3; transform: scale(1.05);}";
        html << ".container {background: #ffffff; padding: 50px; border-radius: 20px; display: inline-block; box-shadow: 0 10px 30px rgba(0,0,0,0.1); width: 60%;}</style></head><body>";
        html << "<div class='container'><h2 style='color: #28a745;'>Focus Mode is Active!</h2><p style='font-size: 18px; color: #6c757d; margin-bottom: 30px;'>Access granted only to approved websites:</p>";
        
        if (allowedWebs.isEmpty()) { html << "<p style='color:#dc3545;'>Your Allow List is empty!</p>"; } 
        else {
            for (const QString& web : allowedWebs) {
                QString cleanWeb = web; int starPos = cleanWeb.indexOf("/*");
                if (starPos != -1) cleanWeb = cleanWeb.left(starPos);
                html << "<a href='https://" << cleanWeb << "' target='_blank'>" << cleanWeb << "</a>";
            }
        }
        html << "</div></body></html>";
        file.close();
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(htmlPath));
}

// ==========================================
// OVERLAY WINDOW CLASS (BEAUTIFUL POPUP)
// ==========================================
class OverlayWindow : public QWidget {
public:
    QLabel* textLabel;
    QTimer* hideTimer;

    OverlayWindow(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground);
        
        resize(700, 200);
        QVBoxLayout* layout = new QVBoxLayout(this);
        textLabel = new QLabel(this);
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setWordWrap(true);
        textLabel->setFont(QFont("Segoe UI", 18, QFont::Bold));
        layout->addWidget(textLabel);
        
        hideTimer = new QTimer(this);
        connect(hideTimer, &QTimer::timeout, this, &OverlayWindow::hideOverlay);
    }

    void showMessage(int type) {
        QString quote;
        if (type == 1) {
            quote = islamicQuotes[rand() % islamicQuotes.size()];
            setStyleSheet("QWidget { background-color: #093d1f; border: 6px solid #f1c40f; border-radius: 15px; } QLabel { color: #ffffff; border: none; }");
        } else {
            quote = timeQuotes[rand() % timeQuotes.size()];
            setStyleSheet("QWidget { background-color: #1a252f; border: 6px solid #3498db; border-radius: 15px; } QLabel { color: #ffffff; border: none; }");
        }
        
        textLabel->setText(quote);
        
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->geometry();
        move((screenGeometry.width() - width()) / 2, (screenGeometry.height() - height()) / 2);

        isOverlayVisible = true;
        show();
        hideTimer->start(8000); 
    }

    void hideOverlay() {
        hide();
        hideTimer->stop();
        isOverlayVisible = false;
    }
};

// ==========================================
// MAIN APPLICATION WINDOW
// ==========================================
class FocusApp : public QMainWindow {
public:
    OverlayWindow* overlay;
    QSystemTrayIcon* trayIcon;
    QTimer* coreTimer;
    
    QLineEdit* passEdit;
    QSpinBox* hrSpin;
    QSpinBox* minSpin;
    QPushButton* btnStart;
    QPushButton* btnStop;
    QLabel* lblTimeLeft;
    
    QRadioButton* rbBlock;
    QRadioButton* rbAllow;
    
    QListWidget* listBlockApps;
    QListWidget* listBlockWebs;
    QListWidget* listRunning;
    QListWidget* listAllowApps;
    QListWidget* listAllowWebs;

    QLineEdit* inputBlockApp;
    QComboBox* inputBlockWeb;
    QLineEdit* inputAllowApp;
    QComboBox* inputAllowWeb;

    FocusApp() {
        setWindowTitle("Focus Mode Manager - Pro Edition");
        resize(1000, 700);
        
        setStyleSheet(R"(
            QMainWindow { background-color: #f4f6f9; }
            QLabel { font-family: 'Segoe UI'; font-size: 14px; color: #2c3e50; font-weight: 500; }
            QLineEdit, QSpinBox, QComboBox, QListWidget { 
                background-color: #ffffff; border: 1px solid #ced4da; border-radius: 6px; padding: 5px; font-size: 14px; color: #495057;
            }
            QListWidget::item { padding: 5px; }
            QListWidget::item:selected { background-color: #007bff; color: white; }
            QPushButton { 
                font-family: 'Segoe UI'; font-weight: bold; font-size: 14px; color: white; 
                background-color: #007bff; border-radius: 6px; padding: 8px 15px; 
            }
            QPushButton:hover { background-color: #0056b3; }
            QPushButton#btnStart { background-color: #28a745; }
            QPushButton#btnStart:hover { background-color: #218838; }
            QPushButton#btnStop { background-color: #dc3545; }
            QPushButton#btnStop:hover { background-color: #c82333; }
            QPushButton#btnRemove { background-color: #6c757d; }
            QPushButton#btnRemove:hover { background-color: #5a6268; }
            QRadioButton { font-size: 14px; font-weight: bold; color: #2c3e50; }
        )");

        QWidget* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);

        // --- TOP BAR ---
        QHBoxLayout* topLayout = new QHBoxLayout();
        topLayout->addWidget(new QLabel("Password:"));
        passEdit = new QLineEdit(); passEdit->setEchoMode(QLineEdit::Password); passEdit->setFixedWidth(120);
        topLayout->addWidget(passEdit);

        topLayout->addSpacing(20);
        topLayout->addWidget(new QLabel("Time (H:M):"));
        hrSpin = new QSpinBox(); hrSpin->setRange(0, 24); hrSpin->setFixedWidth(60);
        topLayout->addWidget(hrSpin);
        topLayout->addWidget(new QLabel(":"));
        minSpin = new QSpinBox(); minSpin->setRange(0, 59); minSpin->setFixedWidth(60);
        topLayout->addWidget(minSpin);

        topLayout->addSpacing(30);
        btnStart = new QPushButton("START FOCUS"); btnStart->setObjectName("btnStart");
        btnStop = new QPushButton("STOP FOCUS"); btnStop->setObjectName("btnStop");
        topLayout->addWidget(btnStart);
        topLayout->addWidget(btnStop);
        
        topLayout->addStretch();
        lblTimeLeft = new QLabel(""); 
        lblTimeLeft->setStyleSheet("color: #dc3545; font-size: 18px; font-weight: bold;");
        topLayout->addWidget(lblTimeLeft);
        mainLayout->addLayout(topLayout);

        // --- RADIO BUTTONS ---
        QHBoxLayout* radioLayout = new QHBoxLayout();
        radioLayout->setAlignment(Qt::AlignCenter);
        rbBlock = new QRadioButton("Block List Mode"); rbBlock->setChecked(true);
        rbAllow = new QRadioButton("Allow List Mode");
        radioLayout->addWidget(rbBlock);
        radioLayout->addSpacing(50);
        radioLayout->addWidget(rbAllow);
        mainLayout->addLayout(radioLayout);

        // --- 3 COLUMNS GRID ---
        QGridLayout* grid = new QGridLayout();
        grid->setSpacing(20);

        grid->addWidget(new QLabel("Block Apps (.exe):"), 0, 0);
        QHBoxLayout* h1 = new QHBoxLayout();
        inputBlockApp = new QLineEdit(); h1->addWidget(inputBlockApp);
        QPushButton* btnAddBApp = new QPushButton("ADD"); h1->addWidget(btnAddBApp);
        grid->addLayout(h1, 1, 0);
        listBlockApps = new QListWidget(); grid->addWidget(listBlockApps, 2, 0);
        QPushButton* btnRemBApp = new QPushButton("Remove Selected"); btnRemBApp->setObjectName("btnRemove");
        grid->addWidget(btnRemBApp, 3, 0);

        grid->addWidget(new QLabel("Block Websites:"), 4, 0);
        QHBoxLayout* h2 = new QHBoxLayout();
        inputBlockWeb = new QComboBox(); inputBlockWeb->setEditable(true); h2->addWidget(inputBlockWeb);
        QPushButton* btnAddBWeb = new QPushButton("ADD"); h2->addWidget(btnAddBWeb);
        grid->addLayout(h2, 5, 0);
        listBlockWebs = new QListWidget(); grid->addWidget(listBlockWebs, 6, 0);
        QPushButton* btnRemBWeb = new QPushButton("Remove Selected"); btnRemBWeb->setObjectName("btnRemove");
        grid->addWidget(btnRemBWeb, 7, 0);

        grid->addWidget(new QLabel("Running Apps (Select -> Add):"), 0, 1);
        listRunning = new QListWidget(); grid->addWidget(listRunning, 1, 1, 6, 1);
        QPushButton* btnAddRunning = new QPushButton("Add to Current List");
        grid->addWidget(btnAddRunning, 7, 1);

        grid->addWidget(new QLabel("Allow Apps (.exe):"), 0, 2);
        QHBoxLayout* h3 = new QHBoxLayout();
        inputAllowApp = new QLineEdit(); h3->addWidget(inputAllowApp);
        QPushButton* btnAddAApp = new QPushButton("ADD"); h3->addWidget(btnAddAApp);
        grid->addLayout(h3, 1, 2);
        listAllowApps = new QListWidget(); grid->addWidget(listAllowApps, 2, 2);
        QPushButton* btnRemAApp = new QPushButton("Remove Selected"); btnRemAApp->setObjectName("btnRemove");
        grid->addWidget(btnRemAApp, 3, 2);

        grid->addWidget(new QLabel("Allow Websites:"), 4, 2);
        QHBoxLayout* h4 = new QHBoxLayout();
        inputAllowWeb = new QComboBox(); inputAllowWeb->setEditable(true); h4->addWidget(inputAllowWeb);
        QPushButton* btnAddAWeb = new QPushButton("ADD"); h4->addWidget(btnAddAWeb);
        grid->addLayout(h4, 5, 2);
        listAllowWebs = new QListWidget(); grid->addWidget(listAllowWebs, 6, 2);
        QPushButton* btnRemAWeb = new QPushButton("Remove Selected"); btnRemAWeb->setObjectName("btnRemove");
        grid->addWidget(btnRemAWeb, 7, 2);

        mainLayout->addLayout(grid);

        QStringList popSites = {"facebook.com", "youtube.com", "instagram.com", "tiktok.com", "reddit.com", "netflix.com"};
        inputBlockWeb->addItems(popSites); inputAllowWeb->addItems(popSites);
        inputBlockWeb->setCurrentText(""); inputAllowWeb->setCurrentText("");

        overlay = new OverlayWindow();
        setupTrayIcon();

        LoadDataList("blocked_apps.txt", blockedApps, listBlockApps);
        LoadDataList("blocked_webs.txt", blockedWebs, listBlockWebs);
        LoadDataList("allowed_apps.txt", allowedApps, listAllowApps);
        LoadDataList("allowed_webs.txt", allowedWebs, listAllowWebs);
        refreshRunningApps();

        // SIGNALS AND SLOTS
        connect(passEdit, &QLineEdit::textChanged, [=](const QString &text){
            bool hasPass = !text.isEmpty();
            hrSpin->setDisabled(hasPass); minSpin->setDisabled(hasPass);
        });
        auto timeChange = [=](int){
            bool hasTime = (hrSpin->value() > 0 || minSpin->value() > 0);
            passEdit->setDisabled(hasTime);
        };
        connect(hrSpin, QOverload<int>::of(&QSpinBox::valueChanged), timeChange);
        connect(minSpin, QOverload<int>::of(&QSpinBox::valueChanged), timeChange);

        auto toggleMode = [=](){
            bool isAllow = rbAllow->isChecked();
            inputAllowApp->setEnabled(isAllow); btnAddAApp->setEnabled(isAllow); listAllowApps->setEnabled(isAllow); btnRemAApp->setEnabled(isAllow);
            inputAllowWeb->setEnabled(isAllow); btnAddAWeb->setEnabled(isAllow); listAllowWebs->setEnabled(isAllow); btnRemAWeb->setEnabled(isAllow);
            
            inputBlockApp->setEnabled(!isAllow); btnAddBApp->setEnabled(!isAllow); listBlockApps->setEnabled(!isAllow); btnRemBApp->setEnabled(!isAllow);
            inputBlockWeb->setEnabled(!isAllow); btnAddBWeb->setEnabled(!isAllow); listBlockWebs->setEnabled(!isAllow); btnRemBWeb->setEnabled(!isAllow);
        };
        connect(rbBlock, &QRadioButton::toggled, toggleMode); connect(rbAllow, &QRadioButton::toggled, toggleMode);
        toggleMode(); 

        connect(btnAddBApp, &QPushButton::clicked, [=](){ addToList(inputBlockApp, listBlockApps, blockedApps, "blocked_apps.txt"); });
        connect(btnAddAApp, &QPushButton::clicked, [=](){ addToList(inputAllowApp, listAllowApps, allowedApps, "allowed_apps.txt"); });
        connect(btnRemBApp, &QPushButton::clicked, [=](){ remFromList(listBlockApps, blockedApps, "blocked_apps.txt"); });
        connect(btnRemAApp, &QPushButton::clicked, [=](){ remFromList(listAllowApps, allowedApps, "allowed_apps.txt"); });
        
        connect(btnAddBWeb, &QPushButton::clicked, [=](){ addToListCombo(inputBlockWeb, listBlockWebs, blockedWebs, "blocked_webs.txt"); });
        connect(btnAddAWeb, &QPushButton::clicked, [=](){ addToListCombo(inputAllowWeb, listAllowWebs, allowedWebs, "allowed_webs.txt"); });
        connect(btnRemBWeb, &QPushButton::clicked, [=](){ remFromList(listBlockWebs, blockedWebs, "blocked_webs.txt"); });
        connect(btnRemAWeb, &QPushButton::clicked, [=](){ remFromList(listAllowWebs, allowedWebs, "allowed_webs.txt"); });

        connect(btnAddRunning, &QPushButton::clicked, [=](){
            QListWidgetItem* it = listRunning->currentItem();
            if(!it) { QMessageBox::information(this, "Info", "Select an app first!"); return; }
            QString app = it->text();
            if(rbAllow->isChecked()){
                if(!allowedApps.contains(app, Qt::CaseInsensitive)){
                    allowedApps.append(app); listAllowApps->addItem(app); SaveDataList("allowed_apps.txt", allowedApps);
                }
            } else {
                if(!blockedApps.contains(app, Qt::CaseInsensitive)){
                    blockedApps.append(app); listBlockApps->addItem(app); SaveDataList("blocked_apps.txt", blockedApps);
                }
            }
        });

        connect(btnStart, &QPushButton::clicked, this, &FocusApp::startFocus);
        connect(btnStop, &QPushButton::clicked, this, &FocusApp::stopFocus);

        coreTimer = new QTimer(this);
        connect(coreTimer, &QTimer::timeout, this, &FocusApp::coreLoop);
        coreTimer->start(1000);
    }

    void setupTrayIcon() {
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setIcon(QIcon("icon.ico")); 
        
        QMenu* menu = new QMenu(this);
        QAction* restoreAct = new QAction("Restore", this);
        connect(restoreAct, &QAction::triggered, this, &QWidget::showNormal);
        menu->addAction(restoreAct);
        
        trayIcon->setContextMenu(menu);
        connect(trayIcon, &QSystemTrayIcon::activated, [=](QSystemTrayIcon::ActivationReason reason){
            if(reason == QSystemTrayIcon::DoubleClick) showNormal();
        });
        trayIcon->show();
    }

    // উইন্ডো ক্লোজ ইভেন্ট 
    void closeEvent(QCloseEvent *event) override { 
        event->ignore(); 
        hide(); 
    }

    void refreshRunningApps() {
        listRunning->clear();
        QStringList rProc = GetRunningProcesses();
        for (const QString& proc : rProc) {
            QString lp = proc.toLower();
            if (!hiddenUIProcesses.contains(lp)) listRunning->addItem(proc);
        }
    }

    void addToList(QLineEdit* input, QListWidget* lw, QStringList& vec, QString file) {
        QString text = input->text().trimmed();
        if(!text.isEmpty() && !vec.contains(text, Qt::CaseInsensitive)){
            vec.append(text); lw->addItem(text); SaveDataList(file, vec); input->clear();
        }
    }
    void addToListCombo(QComboBox* input, QListWidget* lw, QStringList& vec, QString file) {
        QString text = input->currentText().trimmed();
        if(!text.isEmpty() && !vec.contains(text, Qt::CaseInsensitive)){
            vec.append(text); lw->addItem(text); SaveDataList(file, vec); input->setCurrentText("");
        }
    }
    void remFromList(QListWidget* lw, QStringList& vec, QString file) {
        QListWidgetItem* it = lw->takeItem(lw->currentRow());
        if(it){ vec.removeAll(it->text()); delete it; SaveDataList(file, vec); }
    }

    void startFocus() {
        QString pass = passEdit->text();
        int hrs = hrSpin->value(); int mins = minSpin->value();
        focusTimeTotalSeconds = (hrs * 3600) + (mins * 60);
        useAllowMode = rbAllow->isChecked();

        if (!isSessionActive) {
            if (!pass.isEmpty()) { currentSessionPass = pass; isPassMode = true; isTimeMode = false; isSessionActive = true; } 
            else if (focusTimeTotalSeconds > 0) { isTimeMode = true; isPassMode = false; isSessionActive = true; timerTicks = 0; }

            if (isSessionActive) {
                QMessageBox::information(this, "Success", "Focus Mode Started!\nWindow will hide in tray.");
                passEdit->clear(); hide();
            } else { QMessageBox::warning(this, "Error", "Please enter a password OR set time to start!"); }
        } else { QMessageBox::information(this, "Info", "Focus mode is already active!"); }
    }

    void stopFocus() {
        if (isSessionActive) {
            if (isTimeMode) { QMessageBox::warning(this, "Locked", "Time mode is active. You must wait!"); } 
            else if (isPassMode) {
                if (currentSessionPass == passEdit->text()) {
                    isSessionActive = false; lblTimeLeft->setText(""); 
                    QMessageBox::information(this, "Success", "Focus session stopped.");
                    passEdit->setEnabled(true); hrSpin->setEnabled(true); minSpin->setEnabled(true); passEdit->clear();
                } else { QMessageBox::critical(this, "Error", "Wrong password!"); }
            }
        } else { QMessageBox::information(this, "Info", "Focus session is not active."); }
    }

    // ==========================================
    // CORE FILTERING LOGIC 
    // ==========================================
    void coreLoop() {
        if (!isOverlayVisible) {
            HWND hActive = GetForegroundWindow();
            if (hActive && hActive != (HWND)overlay->winId()) {
                WCHAR title[512];
                if (GetWindowTextW(hActive, title, 512) > 0) {
                    QString sTitle = QString::fromWCharArray(title).toLower();
                    for (const QString& keyword : explicitKeywords) {
                        if (sTitle.contains(keyword)) { ShowWindow(hActive, SW_MINIMIZE); overlay->showMessage(1); return; }
                    }
                    for (const QString& web : defaultAdultWebs) {
                        if (sTitle.contains(web)) { CloseActiveTabAndMinimize(hActive); overlay->showMessage(1); return; }
                    }
                }
            }
        }

        if (isSessionActive) {
            if (isTimeMode && focusTimeTotalSeconds > 0) {
                timerTicks++;
                int timeLeft = focusTimeTotalSeconds - timerTicks;
                if (timeLeft <= 0) {
                    isSessionActive = false; isTimeMode = false; lblTimeLeft->setText("");
                    QMessageBox::information(this, "Success", "Focus time is over! Settings & Task Manager are unlocked.");
                    passEdit->setEnabled(true); hrSpin->setEnabled(true); minSpin->setEnabled(true);
                    return;
                }
                int h = timeLeft / 3600; int m = (timeLeft % 3600) / 60; int s = timeLeft % 60;
                lblTimeLeft->setText(QString("Time Left: %1:%2:%3").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')));
            }

            HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe = {sizeof(pe)};
            if (Process32FirstW(hSnap, &pe)) {
                do {
                    QString n = QString::fromWCharArray(pe.szExeFile).toLower();
                    if (n == "taskmgr.exe") {
                        HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                        if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); } continue;
                    }
                    if (defaultSystemBlocks.contains(n)) {
                        HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                        if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); }
                    }
                    if (useAllowMode) {
                        bool isSystemApp = systemApps.contains(n);
                        bool isAllowed = allowedApps.contains(n, Qt::CaseInsensitive);
                        if (!isSystemApp && !isAllowed) {
                            HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                            if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); }
                        }
                    } else {
                        if (blockedApps.contains(n, Qt::CaseInsensitive)) {
                            HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                            if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); }
                        }
                    }
                } while (Process32NextW(hSnap, &pe));
            }
            CloseHandle(hSnap);

            if (!isOverlayVisible) {
                HWND hActive = GetForegroundWindow();
                if (hActive && hActive != (HWND)overlay->winId()) {
                    WCHAR title[512];
                    if (GetWindowTextW(hActive, title, 512) > 0) {
                        QString sTitle = QString::fromWCharArray(title).toLower();
                        if (sTitle.contains("allowed websites")) return;
                        for (const QString& sysTitle : defaultSystemTitles) { if (sTitle.contains(sysTitle)) { ShowWindow(hActive, SW_MINIMIZE); return; } }

                        bool isBrowser = (sTitle.contains("google chrome") || sTitle.contains("edge") || sTitle.contains("firefox") || sTitle.contains("brave"));

                        if (useAllowMode && isBrowser) {
                            bool isAllowed = false;
                            for (const QString& web : allowedWebs) {
                                QString cleanWeb = CleanURL(web); 
                                if (!cleanWeb.isEmpty() && sTitle.contains(cleanWeb)) { isAllowed = true; break; }
                            }
                            if (!isAllowed) { CloseActiveTabAndMinimize(hActive); ShowAllowedWebsitesPage(); }
                        } else if (!useAllowMode) {
                            for (const QString& web : blockedWebs) {
                                QString cleanWeb = CleanURL(web); 
                                if (!cleanWeb.isEmpty() && sTitle.contains(cleanWeb)) {
                                    CloseActiveTabAndMinimize(hActive); 
                                    overlay->showMessage(2); 
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
};

int main(int argc, char *argv[]) {
    // Admin Check
    if (!IsRunAsAdmin()) {
        WCHAR path[MAX_PATH]; GetModuleFileNameW(NULL, path, MAX_PATH);
        SHELLEXECUTEINFOW sei = { 0 }; 
        sei.cbSize = sizeof(sei); 
        sei.lpVerb = L"runas"; 
        sei.lpFile = path; 
        sei.nShow = SW_NORMAL;
        if (!ShellExecuteExW(&sei)) { MessageBoxW(NULL, L"Requires Administrator privileges!", L"Error", MB_ICONERROR); }
        return 0; 
    }

    QApplication app(argc, argv);
    QFont defaultFont("Segoe UI", 10);
    app.setFont(defaultFont);

    FocusApp window;
    window.show();

    return app.exec();
}
