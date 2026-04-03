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
#include <QCloseEvent>

class FocusWindow : public QMainWindow
{
public:
    FocusWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
    {
        setWindowTitle("RasFocus Pro");
        resize(1460, 900);
        setMinimumSize(1280, 820);

        tray = new QSystemTrayIcon(this);
        tray->setToolTip("RasFocus Pro");
        auto *trayMenu = new QMenu(this);
        trayMenu->addAction("Show", this, [this]() {
            showNormal();
            raise();
            activateWindow();
        });
        trayMenu->addSeparator();
        trayMenu->addAction("Quit", qApp, &QApplication::quit);
        tray->setContextMenu(trayMenu);
        tray->show();

        auto *central = new QWidget(this);
        central->setObjectName("centralRoot");
        setCentralWidget(central);

        auto *root = new QHBoxLayout(central);
        root->setContentsMargins(18, 18, 18, 18);
        root->setSpacing(18);

        root->addWidget(buildSidebar(), 0);
        root->addWidget(buildMainArea(), 1);

        applyTheme();
        setupConnections();
        seedDemoData();

        statusTimer = new QTimer(this);
        statusTimer->start(1000);
        QObject::connect(statusTimer, &QTimer::timeout, this, [this]() {
            if (!sessionActive) {
                return;
            }

            if (pomodoroMode) {
                ++pomodoroTicks;
                int target = pomodoroBreak ? (2 * 60) : (25 * 60);
                int remain = qMax(0, target - pomodoroTicks);
                timerLabel->setText(pomodoroBreak
                    ? QString("Break: %1:%2").arg(remain / 60, 2, 10, QChar('0')).arg(remain % 60, 2, 10, QChar('0'))
                    : QString("Focus: %1:%2").arg(remain / 60, 2, 10, QChar('0')).arg(remain % 60, 2, 10, QChar('0')));

                if (!pomodoroBreak && pomodoroTicks >= 25 * 60) {
                    pomodoroBreak = true;
                    pomodoroTicks = 0;
                    QMessageBox::information(this, "Pomodoro", "Focus phase complete. Break started.");
                } else if (pomodoroBreak && pomodoroTicks >= 2 * 60) {
                    pomodoroBreak = false;
                    pomodoroTicks = 0;
                    QMessageBox::information(this, "Pomodoro", "Break finished. Focus resumed.");
                }
            } else if (timeMode) {
                ++timerTicks;
                int remain = qMax(0, focusTimeTotalSeconds - timerTicks);
                timerLabel->setText(
                    QString("Time Left: %1:%2:%3")
                    .arg(remain / 3600, 2, 10, QChar('0'))
                    .arg((remain % 3600) / 60, 2, 10, QChar('0'))
                    .arg(remain % 60, 2, 10, QChar('0'))
                );
                if (remain <= 0) {
                    sessionActive = false;
                    timeMode = false;
                    timerTicks = 0;
                    focusTimeTotalSeconds = 0;
                    timerLabel->setText("00:00:00");
                    QMessageBox::information(this, "Done", "Focus time is over.");
                    syncUiState();
                }
            } else if (passMode) {
                timerLabel->setText("Focus Active (Pass Lock)");
            }
        });

        syncUiState();
    }

protected:
    void closeEvent(QCloseEvent *event) override
    {
        if (tray && tray->isVisible()) {
            hide();
            tray->showMessage("RasFocus Pro", "App minimized to tray.");
            event->ignore();
            return;
        }
        QMainWindow::closeEvent(event);
    }

private:
    QWidget *buildSidebar()
    {
        auto *panel = new QWidget;
        panel->setObjectName("sidebar");
        panel->setFixedWidth(270);

        auto *layout = new QVBoxLayout(panel);
        layout->setContentsMargins(18, 18, 18, 18);
        layout->setSpacing(16);

        auto *brandWrap = new QWidget;
        auto *brandLayout = new QVBoxLayout(brandWrap);
        brandLayout->setContentsMargins(0, 0, 0, 0);
        brandLayout->setSpacing(6);

        auto *title = new QLabel("RasFocus");
        title->setObjectName("brandTitle");

        auto *subtitle = new QLabel("Premium Focus Control");
        subtitle->setObjectName("brandSubTitle");

        auto *badge = new QLabel(" FocusMe-style Premium UI ");
        badge->setObjectName("premiumBadge");

        brandLayout->addWidget(title);
        brandLayout->addWidget(subtitle);
        brandLayout->addWidget(badge, 0, Qt::AlignLeft);

        profileCard = new QWidget;
        profileCard->setObjectName("softCard");
        auto *profileLayout = new QVBoxLayout(profileCard);
        profileLayout->setContentsMargins(14, 14, 14, 14);
        profileLayout->setSpacing(10);

        auto *profileTitle = new QLabel("Profile");
        profileTitle->setObjectName("cardTitle");

        profileNameEdit = new QLineEdit;
        profileNameEdit->setPlaceholderText("Profile Name");

        saveProfileBtn = new QPushButton("SAVE PROFILE");
        saveProfileBtn->setObjectName("greenButton");

        profileLayout->addWidget(profileTitle);
        profileLayout->addWidget(profileNameEdit);
        profileLayout->addWidget(saveProfileBtn);

        statusCard = new QWidget;
        statusCard->setObjectName("softCard");
        auto *statusLayout = new QVBoxLayout(statusCard);
        statusLayout->setContentsMargins(14, 14, 14, 14);
        statusLayout->setSpacing(10);

        auto *statusTitle = new QLabel("License & Session");
        statusTitle->setObjectName("cardTitle");

        trialStatusLabel = new QLabel("TRIAL: 7 Days");
        trialStatusLabel->setObjectName("statusValue");

        adminMsgLabel = new QLabel("Admin channel idle");
        adminMsgLabel->setWordWrap(true);
        adminMsgLabel->setObjectName("mutedText");

        timerLabel = new QLabel("00:00:00");
        timerLabel->setObjectName("heroTimer");

        statusLayout->addWidget(statusTitle);
        statusLayout->addWidget(trialStatusLabel);
        statusLayout->addWidget(timerLabel);
        statusLayout->addWidget(adminMsgLabel);

        toolsCard = new QWidget;
        toolsCard->setObjectName("softCard");
        auto *toolsLayout = new QVBoxLayout(toolsCard);
        toolsLayout->setContentsMargins(14, 14, 14, 14);
        toolsLayout->setSpacing(10);

        auto *toolsTitle = new QLabel("Quick Actions");
        toolsTitle->setObjectName("cardTitle");

        eyeCureBtn = new QPushButton("EYE CURE");
        eyeCureBtn->setObjectName("purpleButton");

        guideBtn = new QPushButton("USER GUIDE");
        guideBtn->setObjectName("cyanButton");

        liveChatBtn = new QPushButton("LIVE CHAT");
        liveChatBtn->setObjectName("pinkButton");

        upgradeBtn = new QPushButton("PREMIUM UPGRADE");
        upgradeBtn->setObjectName("goldButton");

        toolsLayout->addWidget(toolsTitle);
        toolsLayout->addWidget(eyeCureBtn);
        toolsLayout->addWidget(guideBtn);
        toolsLayout->addWidget(liveChatBtn);
        toolsLayout->addWidget(upgradeBtn);

        layout->addWidget(brandWrap);
        layout->addWidget(profileCard);
        layout->addWidget(statusCard);
        layout->addWidget(toolsCard);
        layout->addStretch(1);

        return panel;
    }

    QWidget *buildMainArea()
    {
        auto *wrap = new QWidget;
        auto *mainLayout = new QVBoxLayout(wrap);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(18);

        mainLayout->addWidget(buildTopBar());

        auto *bodyGrid = new QGridLayout;
        bodyGrid->setHorizontalSpacing(18);
        bodyGrid->setVerticalSpacing(18);

        bodyGrid->addWidget(buildSessionCard(), 0, 0, 1, 3);
        bodyGrid->addWidget(buildBlockAppsCard(), 1, 0);
        bodyGrid->addWidget(buildRunningAppsCard(), 1, 1);
        bodyGrid->addWidget(buildAllowAppsCard(), 1, 2);
        bodyGrid->addWidget(buildBlockWebsCard(), 2, 0);
        bodyGrid->addWidget(buildRunningHintCard(), 2, 1);
        bodyGrid->addWidget(buildAllowWebsCard(), 2, 2);

        bodyGrid->setColumnStretch(0, 1);
        bodyGrid->setColumnStretch(1, 1);
        bodyGrid->setColumnStretch(2, 1);
        bodyGrid->setRowStretch(1, 1);
        bodyGrid->setRowStretch(2, 1);

        auto *bodyWidget = new QWidget;
        bodyWidget->setLayout(bodyGrid);

        mainLayout->addWidget(bodyWidget, 1);
        return wrap;
    }

    QWidget *buildTopBar()
    {
        auto *topCard = new QWidget;
        topCard->setObjectName("glassCard");
        auto *layout = new QHBoxLayout(topCard);
        layout->setContentsMargins(18, 16, 18, 16);
        layout->setSpacing(16);

        auto *left = new QVBoxLayout;
        left->setSpacing(4);

        auto *headline = new QLabel("Stay locked on the right workload");
        headline->setObjectName("headline");

        auto *sub = new QLabel("Beautiful premium control panel — same feature map, upgraded operator interface.");
        sub->setObjectName("mutedText");

        left->addWidget(headline);
        left->addWidget(sub);

        auto *right = new QHBoxLayout;
        right->setSpacing(10);

        pomodoroCheck = new QPushButton("Pomodoro");
        pomodoroCheck->setCheckable(true);
        pomodoroCheck->setObjectName("toggleButton");

        adBlockCheck = new QPushButton("Ad Blocker");
        adBlockCheck->setCheckable(true);
        adBlockCheck->setObjectName("toggleButton");

        reelsCheck = new QPushButton("FB Reels");
        reelsCheck->setCheckable(true);
        reelsCheck->setObjectName("toggleButton");

        shortsCheck = new QPushButton("YT Shorts");
        shortsCheck->setCheckable(true);
        shortsCheck->setObjectName("toggleButton");

        right->addWidget(pomodoroCheck);
        right->addWidget(adBlockCheck);
        right->addWidget(reelsCheck);
        right->addWidget(shortsCheck);

        layout->addLayout(left, 1);
        layout->addLayout(right, 0);

        return topCard;
    }

    QWidget *buildSessionCard()
    {
        auto *card = new QWidget;
        card->setObjectName("softCard");

        auto *layout = new QGridLayout(card);
        layout->setContentsMargins(18, 18, 18, 18);
        layout->setHorizontalSpacing(16);
        layout->setVerticalSpacing(14);

        auto *title = new QLabel("Session Control");
        title->setObjectName("sectionTitle");

        auto *desc = new QLabel("Keep logic mapping exactly aligned: password mode, timer mode, pomodoro mode, block/allow mode.");
        desc->setObjectName("mutedText");
        desc->setWordWrap(true);

        blockModeRadio = new QRadioButton("Block List");
        allowModeRadio = new QRadioButton("Allow List");
        blockModeRadio->setChecked(true);

        auto *modeGroup = new QButtonGroup(this);
        modeGroup->addButton(blockModeRadio);
        modeGroup->addButton(allowModeRadio);

        passwordEdit = new QLineEdit;
        passwordEdit->setPlaceholderText("Self Control Password");
        passwordEdit->setEchoMode(QLineEdit::Password);

        hourSpin = new QSpinBox;
        hourSpin->setRange(0, 99);
        hourSpin->setPrefix("Hr ");

        minuteSpin = new QSpinBox;
        minuteSpin->setRange(0, 59);
        minuteSpin->setPrefix("Min ");

        startBtn = new QPushButton("START FOCUS");
        startBtn->setObjectName("greenButton");

        stopBtn = new QPushButton("STOP");
        stopBtn->setObjectName("redButton");

        auto *radioWrap = new QHBoxLayout;
        radioWrap->addWidget(blockModeRadio);
        radioWrap->addWidget(allowModeRadio);
        radioWrap->addStretch(1);

        layout->addWidget(title, 0, 0, 1, 6);
        layout->addWidget(desc, 1, 0, 1, 6);
        layout->addLayout(radioWrap, 2, 0, 1, 2);
        layout->addWidget(passwordEdit, 2, 2, 1, 2);
        layout->addWidget(hourSpin, 2, 4);
        layout->addWidget(minuteSpin, 2, 5);
        layout->addWidget(startBtn, 3, 4);
        layout->addWidget(stopBtn, 3, 5);

        return card;
    }

    QWidget *buildBlockAppsCard()
    {
        auto *card = makeListCard(
            "Block Apps",
            "Apps in this list will be force-closed when focus is active.",
            blockAppInput,
            addBlockAppBtn,
            blockAppList,
            removeBlockAppBtn
        );
        addBlockAppBtn->setText("ADD");
        removeBlockAppBtn->setText("REMOVE");
        return card;
    }

    QWidget *buildRunningAppsCard()
    {
        auto *card = new QWidget;
        card->setObjectName("softCard");
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(18, 18, 18, 18);
        layout->setSpacing(12);

        auto *title = new QLabel("Running Apps");
        title->setObjectName("sectionTitle");

        auto *sub = new QLabel("Snapshot of running or common productivity/distraction executables.");
        sub->setObjectName("mutedText");
        sub->setWordWrap(true);

        addFromRunningBtn = new QPushButton("ADD TO CURRENT LIST");
        addFromRunningBtn->setObjectName("blueButton");

        runningAppsList = new QListWidget;
        runningAppsList->setSelectionMode(QAbstractItemView::SingleSelection);

        layout->addWidget(title);
        layout->addWidget(sub);
        layout->addWidget(addFromRunningBtn);
        layout->addWidget(runningAppsList, 1);

        return card;
    }

    QWidget *buildAllowAppsCard()
    {
        auto *card = makeListCard(
            "Allow Apps",
            "Only these apps remain allowed during allow-list mode.",
            allowAppInput,
            addAllowAppBtn,
            allowAppList,
            removeAllowAppBtn
        );
        addAllowAppBtn->setText("ADD");
        removeAllowAppBtn->setText("REMOVE");
        return card;
    }

    QWidget *buildBlockWebsCard()
    {
        auto *card = makeWebCard(
            "Block Websites",
            "Any matching browser title/url gets blocked in block-list mode.",
            blockWebInput,
            addBlockWebBtn,
            blockWebList,
            removeBlockWebBtn
        );
        return card;
    }

    QWidget *buildRunningHintCard()
    {
        auto *card = new QWidget;
        card->setObjectName("glassCard");

        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(18, 18, 18, 18);
        layout->setSpacing(10);

        auto *title = new QLabel("Operator Notes");
        title->setObjectName("sectionTitle");

        auto *b1 = new QLabel("• Existing logic integration point: replace the slot bodies only, UI mapping stays same.");
        auto *b2 = new QLabel("• One-file structure kept intentionally for later EXE packaging.");
        auto *b3 = new QLabel("• FocusMe-style feel achieved with glass surfaces, premium spacing, soft shadows, and status hierarchy.");
        auto *b4 = new QLabel("• Current code is Qt Widgets UI shell ready for your original enforcement engine.");

        for (auto *lbl : {b1, b2, b3, b4}) {
            lbl->setWordWrap(true);
            lbl->setObjectName("mutedText");
            layout->addWidget(lbl);
        }

        layout->addStretch(1);
        return card;
    }

    QWidget *buildAllowWebsCard()
    {
        auto *card = makeWebCard(
            "Allow Websites",
            "Only these domains are allowed in allow-list mode.",
            allowWebInput,
            addAllowWebBtn,
            allowWebList,
            removeAllowWebBtn
        );
        return card;
    }

    QWidget *makeListCard(
        const QString &titleText,
        const QString &subText,
        QLineEdit *&inputRef,
        QPushButton *&addBtnRef,
        QListWidget *&listRef,
        QPushButton *&removeBtnRef)
    {
        auto *card = new QWidget;
        card->setObjectName("softCard");

        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(18, 18, 18, 18);
        layout->setSpacing(12);

        auto *title = new QLabel(titleText);
        title->setObjectName("sectionTitle");

        auto *sub = new QLabel(subText);
        sub->setObjectName("mutedText");
        sub->setWordWrap(true);

        auto *row = new QHBoxLayout;
        row->setSpacing(10);

        inputRef = new QLineEdit;
        inputRef->setPlaceholderText("Enter app name, example: chrome.exe");

        addBtnRef = new QPushButton("ADD");
        addBtnRef->setObjectName("blueButton");

        row->addWidget(inputRef, 1);
        row->addWidget(addBtnRef);

        listRef = new QListWidget;
        listRef->setSelectionMode(QAbstractItemView::SingleSelection);

        removeBtnRef = new QPushButton("REMOVE");
        removeBtnRef->setObjectName("darkButton");

        layout->addWidget(title);
        layout->addWidget(sub);
        layout->addLayout(row);
        layout->addWidget(listRef, 1);
        layout->addWidget(removeBtnRef);

        return card;
    }

    QWidget *makeWebCard(
        const QString &titleText,
        const QString &subText,
        QComboBox *&comboRef,
        QPushButton *&addBtnRef,
        QListWidget *&listRef,
        QPushButton *&removeBtnRef)
    {
        auto *card = new QWidget;
        card->setObjectName("softCard");

        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(18, 18, 18, 18);
        layout->setSpacing(12);

        auto *title = new QLabel(titleText);
        title->setObjectName("sectionTitle");

        auto *sub = new QLabel(subText);
        sub->setObjectName("mutedText");
        sub->setWordWrap(true);

        auto *row = new QHBoxLayout;
        row->setSpacing(10);

        comboRef = new QComboBox;
        comboRef->setEditable(true);
        comboRef->addItems({"facebook.com", "youtube.com", "instagram.com", "twitter.com", "reddit.com"});
        comboRef->setInsertPolicy(QComboBox::NoInsert);

        addBtnRef = new QPushButton("ADD");
        addBtnRef->setObjectName("blueButton");

        row->addWidget(comboRef, 1);
        row->addWidget(addBtnRef);

        listRef = new QListWidget;
        listRef->setSelectionMode(QAbstractItemView::SingleSelection);

        removeBtnRef = new QPushButton("REMOVE");
        removeBtnRef->setObjectName("darkButton");

        layout->addWidget(title);
        layout->addWidget(sub);
        layout->addLayout(row);
        layout->addWidget(listRef, 1);
        layout->addWidget(removeBtnRef);

        return card;
    }

    void applyTheme()
    {
        const QString style = R"(
            * {
                font-family: "Segoe UI", "Inter", "Arial";
                font-size: 14px;
                color: #e5eefc;
            }

            QMainWindow, QWidget#centralRoot {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                    stop:0 #0a1020, stop:0.35 #0d1529, stop:1 #111c33);
            }

            QWidget#sidebar {
                background: rgba(10, 18, 34, 0.88);
                border: 1px solid rgba(255,255,255,0.08);
                border-radius: 28px;
            }

            QWidget#softCard {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                    stop:0 rgba(18, 28, 52, 0.96),
                    stop:1 rgba(15, 24, 44, 0.92));
                border: 1px solid rgba(255,255,255,0.08);
                border-radius: 24px;
            }

            QWidget#glassCard {
                background: rgba(22, 33, 59, 0.72);
                border: 1px solid rgba(255,255,255,0.10);
                border-radius: 24px;
            }

            QLabel#brandTitle {
                font-size: 28px;
                font-weight: 800;
                letter-spacing: 0.5px;
                color: #ffffff;
            }

            QLabel#brandSubTitle {
                font-size: 13px;
                color: #9fb2d2;
            }

            QLabel#premiumBadge {
                background: rgba(130, 90, 255, 0.18);
                color: #d9c8ff;
                border: 1px solid rgba(173, 144, 255, 0.35);
                border-radius: 12px;
                padding: 6px 10px;
                font-size: 12px;
                font-weight: 700;
            }

            QLabel#sectionTitle {
                font-size: 20px;
                font-weight: 800;
                color: #ffffff;
            }

            QLabel#cardTitle {
                font-size: 16px;
                font-weight: 700;
                color: #ffffff;
            }

            QLabel#headline {
                font-size: 24px;
                font-weight: 800;
                color: #ffffff;
            }

            QLabel#mutedText {
                color: #95a8c7;
                font-size: 13px;
            }

            QLabel#statusValue {
                color: #8ef0b1;
                font-size: 15px;
                font-weight: 700;
            }

            QLabel#heroTimer {
                font-size: 32px;
                font-weight: 900;
                color: #ff9b7c;
                padding: 6px 0px 8px 0px;
            }

            QLineEdit, QSpinBox, QComboBox, QListWidget {
                background: rgba(7, 13, 26, 0.72);
                border: 1px solid rgba(255,255,255,0.08);
                border-radius: 14px;
                padding: 11px 13px;
                color: #edf4ff;
                selection-background-color: #4f7cff;
            }

            QLineEdit:focus, QSpinBox:focus, QComboBox:focus, QListWidget:focus {
                border: 1px solid rgba(113, 154, 255, 0.80);
            }

            QListWidget {
                padding: 10px;
            }

            QListWidget::item {
                background: rgba(255,255,255,0.03);
                border-radius: 10px;
                padding: 10px 12px;
                margin: 3px 0px;
            }

            QListWidget::item:selected {
                background: rgba(89, 130, 255, 0.28);
                border: 1px solid rgba(126, 165, 255, 0.50);
            }

            QPushButton {
                background: rgba(70, 94, 138, 0.42);
                border: 1px solid rgba(255,255,255,0.08);
                border-radius: 14px;
                padding: 11px 16px;
                font-weight: 700;
            }

            QPushButton:hover {
                border: 1px solid rgba(255,255,255,0.18);
            }

            QPushButton:pressed {
                padding-top: 12px;
            }

            QPushButton#greenButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #12b76a, stop:1 #0ea55e);
                color: white;
            }

            QPushButton#redButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #ff6b6b, stop:1 #ef4444);
                color: white;
            }

            QPushButton#purpleButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #8b5cf6, stop:1 #6d28d9);
                color: white;
            }

            QPushButton#cyanButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #06b6d4, stop:1 #0891b2);
                color: white;
            }

            QPushButton#pinkButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #ec4899, stop:1 #db2777);
                color: white;
            }

            QPushButton#goldButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #f59e0b, stop:1 #d97706);
                color: white;
            }

            QPushButton#blueButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #4f7cff, stop:1 #3563f6);
                color: white;
            }

            QPushButton#darkButton {
                background: rgba(37, 48, 72, 0.92);
                color: #dbeafe;
            }

            QPushButton#toggleButton {
                background: rgba(255,255,255,0.06);
                color: #dbe7ff;
                border-radius: 16px;
                padding: 10px 16px;
            }

            QPushButton#toggleButton:checked {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #355ee9, stop:1 #5b7cff);
                border: 1px solid rgba(170, 196, 255, 0.65);
            }

            QRadioButton {
                spacing: 8px;
                color: #d9e6ff;
                font-weight: 600;
            }

            QRadioButton::indicator {
                width: 18px;
                height: 18px;
            }

            QRadioButton::indicator:unchecked {
                border-radius: 9px;
                border: 2px solid #7186a9;
                background: transparent;
            }

            QRadioButton::indicator:checked {
                border-radius: 9px;
                border: 2px solid #7aa2ff;
                background: #7aa2ff;
            }

            QScrollBar:vertical {
                background: transparent;
                width: 12px;
                margin: 2px;
            }

            QScrollBar::handle:vertical {
                background: rgba(157, 181, 224, 0.35);
                border-radius: 6px;
                min-height: 26px;
            }

            QScrollBar::add-line:vertical,
            QScrollBar::sub-line:vertical,
            QScrollBar::add-page:vertical,
            QScrollBar::sub-page:vertical {
                background: none;
                height: 0px;
            }
        )";

        setStyleSheet(style);
    }

    void seedDemoData()
    {
        runningAppsList->addItems({
            "chrome.exe", "msedge.exe", "firefox.exe", "code.exe",
            "telegram.exe", "discord.exe", "spotify.exe", "vlc.exe",
            "steam.exe", "notepad.exe", "excel.exe", "winword.exe"
        });

        blockAppList->addItems({"chrome.exe", "telegram.exe"});
        blockWebList->addItems({"facebook.com", "youtube.com"});
        allowAppList->addItems({"code.exe", "notepad.exe"});
        allowWebList->addItems({"github.com", "stackoverflow.com"});
    }

    void setupConnections()
    {
        QObject::connect(saveProfileBtn, &QPushButton::clicked, this, [this]() {
            QMessageBox::information(this, "Profile", "Profile name saved.\n\nOriginal backend sync hook can be placed here.");
        });

        QObject::connect(guideBtn, &QPushButton::clicked, this, [this]() {
            QDesktopServices::openUrl(QUrl("https://example.com"));
        });

        QObject::connect(liveChatBtn, &QPushButton::clicked, this, [this]() {
            QMessageBox::information(this, "Live Chat", "Open your original Live Chat panel logic here.");
        });

        QObject::connect(upgradeBtn, &QPushButton::clicked, this, [this]() {
            QMessageBox::information(this, "Upgrade", "Open your original Premium Upgrade panel logic here.");
        });

        QObject::connect(eyeCureBtn, &QPushButton::clicked, this, [this]() {
            QMessageBox::information(this, "Eye Cure", "Open your original Eye Cure logic here.");
        });

        QObject::connect(addBlockAppBtn, &QPushButton::clicked, this, [this]() {
            addLineEditToList(blockAppInput, blockAppList);
        });
        QObject::connect(addAllowAppBtn, &QPushButton::clicked, this, [this]() {
            addLineEditToList(allowAppInput, allowAppList);
        });
        QObject::connect(addBlockWebBtn, &QPushButton::clicked, this, [this]() {
            addComboToList(blockWebInput, blockWebList);
        });
        QObject::connect(addAllowWebBtn, &QPushButton::clicked, this, [this]() {
            addComboToList(allowWebInput, allowWebList);
        });

        QObject::connect(removeBlockAppBtn, &QPushButton::clicked, this, [this]() {
            deleteCurrentItem(blockAppList);
        });
        QObject::connect(removeAllowAppBtn, &QPushButton::clicked, this, [this]() {
            deleteCurrentItem(allowAppList);
        });
        QObject::connect(removeBlockWebBtn, &QPushButton::clicked, this, [this]() {
            deleteCurrentItem(blockWebList);
        });
        QObject::connect(removeAllowWebBtn, &QPushButton::clicked, this, [this]() {
            deleteCurrentItem(allowWebList);
        });

        QObject::connect(addFromRunningBtn, &QPushButton::clicked, this, [this]() {
            auto *src = runningAppsList->currentItem();
            if (!src) {
                return;
            }
            QListWidget *target = allowModeRadio->isChecked() ? allowAppList : blockAppList;
            const QString text = src->text();
            for (int i = 0; i < target->count(); ++i) {
                if (target->item(i)->text().compare(text, Qt::CaseInsensitive) == 0) {
                    return;
                }
            }
            target->addItem(text);
        });

        QObject::connect(startBtn, &QPushButton::clicked, this, [this]() {
            if (sessionActive) {
                return;
            }

            const QString pass = passwordEdit->text().trimmed();
            const int totalSecs = hourSpin->value() * 3600 + minuteSpin->value() * 60;
            pomodoroMode = pomodoroCheck->isChecked();
            pomodoroBreak = false;
            pomodoroTicks = 0;
            timerTicks = 0;

            if (!pomodoroMode && pass.isEmpty() && totalSecs == 0) {
                QMessageBox::warning(this, "Input Required", "Password, timer, or pomodoro mode must be selected.");
                return;
            }

            sessionActive = true;
            passMode = !pass.isEmpty();
            timeMode = (totalSecs > 0 && !pomodoroMode);
            focusTimeTotalSeconds = totalSecs;
            currentPassword = pass;

            syncUiState();
            hide();

            QMessageBox::information(
                this,
                "Focus Started",
                "Premium UI session started.\n\nOriginal backend enforcement logic should run from this slot."
            );
        });

        QObject::connect(stopBtn, &QPushButton::clicked, this, [this]() {
            if (!sessionActive) {
                return;
            }

            if (passMode && passwordEdit->text() != currentPassword) {
                QMessageBox::critical(this, "Wrong Password", "Password does not match current session lock.");
                return;
            }

            if (timeMode && !passMode) {
                QMessageBox::warning(this, "Locked", "Timer session is active. Wait until it completes.");
                return;
            }

            sessionActive = false;
            passMode = false;
            timeMode = false;
            pomodoroMode = false;
            pomodoroBreak = false;
            pomodoroTicks = 0;
            timerTicks = 0;
            focusTimeTotalSeconds = 0;
            currentPassword.clear();
            timerLabel->setText("00:00:00");
            syncUiState();

            QMessageBox::information(this, "Stopped", "Focus stopped.");
        });

        auto toggleSync = [this]() {
            // Backend hook point for adblock/reels/shorts/pomodoro flags.
        };

        QObject::connect(pomodoroCheck, &QPushButton::toggled, this, [this, toggleSync](bool checked) {
            if (checked) {
                passwordEdit->clear();
                hourSpin->setValue(0);
                minuteSpin->setValue(0);
            }
            toggleSync();
            syncUiState();
        });
        QObject::connect(adBlockCheck, &QPushButton::toggled, this, [toggleSync](bool) { toggleSync(); });
        QObject::connect(reelsCheck, &QPushButton::toggled, this, [toggleSync](bool) { toggleSync(); });
        QObject::connect(shortsCheck, &QPushButton::toggled, this, [toggleSync](bool) { toggleSync(); });

        QObject::connect(blockModeRadio, &QRadioButton::toggled, this, [this](bool) {
            syncUiState();
        });
        QObject::connect(allowModeRadio, &QRadioButton::toggled, this, [this](bool) {
            syncUiState();
        });

        QObject::connect(passwordEdit, &QLineEdit::textChanged, this, [this](const QString &) {
            syncUiState();
        });
    }

    void syncUiState()
    {
        const bool useAllowMode = allowModeRadio->isChecked();
        const bool hasPassword = !passwordEdit->text().isEmpty();
        const bool hasTime = (hourSpin->value() > 0 || minuteSpin->value() > 0);
        const bool isPom = pomodoroCheck->isChecked();

        blockAppInput->setEnabled(!useAllowMode);
        addBlockAppBtn->setEnabled(!useAllowMode);
        blockAppList->setEnabled(!useAllowMode);
        removeBlockAppBtn->setEnabled(!useAllowMode);

        blockWebInput->setEnabled(!useAllowMode);
        addBlockWebBtn->setEnabled(!useAllowMode);
        blockWebList->setEnabled(!useAllowMode);
        removeBlockWebBtn->setEnabled(!useAllowMode);

        allowAppInput->setEnabled(useAllowMode);
        addAllowAppBtn->setEnabled(useAllowMode);
        allowAppList->setEnabled(useAllowMode);
        removeAllowAppBtn->setEnabled(useAllowMode);

        allowWebInput->setEnabled(useAllowMode);
        addAllowWebBtn->setEnabled(useAllowMode);
        allowWebList->setEnabled(useAllowMode);
        removeAllowWebBtn->setEnabled(useAllowMode);

        if (!sessionActive) {
            if (isPom) {
                passwordEdit->setEnabled(false);
                hourSpin->setEnabled(false);
                minuteSpin->setEnabled(false);
            } else {
                passwordEdit->setEnabled(!hasTime);
                hourSpin->setEnabled(!hasPassword);
                minuteSpin->setEnabled(!hasPassword);
            }
            pomodoroCheck->setEnabled(!hasPassword && !hasTime || pomodoroCheck->isChecked());
        } else {
            passwordEdit->setEnabled(passMode);
            hourSpin->setEnabled(false);
            minuteSpin->setEnabled(false);
            pomodoroCheck->setEnabled(false);
        }

        startBtn->setEnabled(!sessionActive);
        stopBtn->setEnabled(sessionActive);
    }

    void addLineEditToList(QLineEdit *edit, QListWidget *list)
    {
        const QString text = edit->text().trimmed();
        if (text.isEmpty()) {
            return;
        }
        for (int i = 0; i < list->count(); ++i) {
            if (list->item(i)->text().compare(text, Qt::CaseInsensitive) == 0) {
                edit->clear();
                return;
            }
        }
        list->addItem(text);
        edit->clear();
    }

    void addComboToList(QComboBox *combo, QListWidget *list)
    {
        const QString text = combo->currentText().trimmed();
        if (text.isEmpty()) {
            return;
        }
        for (int i = 0; i < list->count(); ++i) {
            if (list->item(i)->text().compare(text, Qt::CaseInsensitive) == 0) {
                combo->setEditText(QString());
                return;
            }
        }
        list->addItem(text);
        combo->setEditText(QString());
    }

    void deleteCurrentItem(QListWidget *list)
    {
        delete list->takeItem(list->currentRow());
    }

private:
    QWidget *profileCard = nullptr;
    QWidget *statusCard = nullptr;
    QWidget *toolsCard = nullptr;

    QLineEdit *profileNameEdit = nullptr;
    QPushButton *saveProfileBtn = nullptr;
    QLabel *trialStatusLabel = nullptr;
    QLabel *adminMsgLabel = nullptr;
    QLabel *timerLabel = nullptr;
    QPushButton *eyeCureBtn = nullptr;
    QPushButton *guideBtn = nullptr;
    QPushButton *liveChatBtn = nullptr;
    QPushButton *upgradeBtn = nullptr;

    QPushButton *pomodoroCheck = nullptr;
    QPushButton *adBlockCheck = nullptr;
    QPushButton *reelsCheck = nullptr;
    QPushButton *shortsCheck = nullptr;

    QRadioButton *blockModeRadio = nullptr;
    QRadioButton *allowModeRadio = nullptr;
    QLineEdit *passwordEdit = nullptr;
    QSpinBox *hourSpin = nullptr;
    QSpinBox *minuteSpin = nullptr;
    QPushButton *startBtn = nullptr;
    QPushButton *stopBtn = nullptr;

    QLineEdit *blockAppInput = nullptr;
    QPushButton *addBlockAppBtn = nullptr;
    QListWidget *blockAppList = nullptr;
    QPushButton *removeBlockAppBtn = nullptr;

    QComboBox *blockWebInput = nullptr;
    QPushButton *addBlockWebBtn = nullptr;
    QListWidget *blockWebList = nullptr;
    QPushButton *removeBlockWebBtn = nullptr;

    QListWidget *runningAppsList = nullptr;
    QPushButton *addFromRunningBtn = nullptr;

    QLineEdit *allowAppInput = nullptr;
    QPushButton *addAllowAppBtn = nullptr;
    QListWidget *allowAppList = nullptr;
    QPushButton *removeAllowAppBtn = nullptr;

    QComboBox *allowWebInput = nullptr;
    QPushButton *addAllowWebBtn = nullptr;
    QListWidget *allowWebList = nullptr;
    QPushButton *removeAllowWebBtn = nullptr;

    QSystemTrayIcon *tray = nullptr;
    QTimer *statusTimer = nullptr;

    bool sessionActive = false;
    bool timeMode = false;
    bool passMode = false;
    bool pomodoroMode = false;
    bool pomodoroBreak = false;
    int focusTimeTotalSeconds = 0;
    int timerTicks = 0;
    int pomodoroTicks = 0;
    QString currentPassword;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("RasFocus Pro");
    app.setOrganizationName("RasFocus");

    FocusWindow window;
    window.show();

    return app.exec();
}
