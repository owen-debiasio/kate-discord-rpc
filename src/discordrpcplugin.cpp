#include "discordrpcplugin.h"
#include "discordrpcconfigpage.h"

#include <QDebug>
#include <QTimer>

#include <KConfigGroup>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

#include <discord_rpc.h>

K_PLUGIN_FACTORY_WITH_JSON(DiscordRpcPluginFactory, "discordrpcplugin.json", registerPlugin<DiscordRpcPlugin>();)

DiscordRpcPlugin::DiscordRpcPlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
    , m_updateTimer(this)
{
    initDiscord();
    readConfig();

    connect(&m_updateTimer, &QTimer::timeout, this, &DiscordRpcPlugin::updateStatus);
    m_updateTimer.start(5000);
}

DiscordRpcPlugin::RPCConfig const DiscordRpcPlugin::DEFAULT_CONFIG{
    .detailsText = "Editing {FILENAME}",
    .stateText = "Project: {PROJECT}",
    .showElapsedTime = true,
};

DiscordRpcPlugin::~DiscordRpcPlugin()
{
    m_updateTimer.stop();
    Discord_Shutdown();
}

QObject *DiscordRpcPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    connect(mainWindow, &KTextEditor::MainWindow::viewChanged, this, &DiscordRpcPlugin::updateStatus);
    updateStatus();

    return new QObject(this);
}

KTextEditor::ConfigPage *DiscordRpcPlugin::configPage(int number, QWidget *parent)
{
    if (number == 0) {
        return new DiscordRpcConfigPage(parent, this);
    }
    return nullptr;
}

void DiscordRpcPlugin::readConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("DiscordRPC"));

    auto defaults = DiscordRpcPlugin::DEFAULT_CONFIG;

    m_config.detailsText = config.readEntry("DetailsText", defaults.detailsText);
    m_config.stateText = config.readEntry("StateText", defaults.stateText);
    m_config.showElapsedTime = config.readEntry("ShowElapsedTime", defaults.showElapsedTime);
}

void DiscordRpcPlugin::initDiscord()
{
    DiscordEventHandlers handlers{
        .ready =
            [](const DiscordUser * /*request*/) {
                qDebug() << "DISCORD RPC CONNECTED";
            },
        .disconnected = [](int /*errorCode*/, const char * /*message*/) {},
        .errored =
            [](int errorCode, const char *message) {
                qWarning() << "DISCORD RPC ERROR: CODE: " << errorCode << "MESSAGE: " << message;
            },
        .joinGame = [](const char * /*joinSecret*/) {},
        .spectateGame = [](const char * /*spectateSecret*/) {},
        .joinRequest = [](const DiscordUser * /*request*/) {},
    };

    Discord_Initialize(DISCORD_ID, &handlers, 1, nullptr);

    m_startTimestamp = QDateTime::currentSecsSinceEpoch();
}

void DiscordRpcPlugin::updateStatus() const
{
    auto *app = KTextEditor::Editor::instance()->application();
    auto *mainWindow = app->activeMainWindow();

    QString fileName = "";
    KTextEditor::View *view = mainWindow->activeView();
    if (view != nullptr) {
        fileName = view->document()->url().fileName();
    }

    QString projectName = "";
    QObject *projectPlugin = mainWindow->pluginView(QStringLiteral("kateprojectplugin"));
    if (projectPlugin != nullptr) {
        projectName = projectPlugin->property("projectName").toString();
    }

    auto formatText = [fileName, projectName](QString text) {
        text.replace("{FILENAME}", fileName);
        text.replace("{PROJECT}", projectName);
        return text.toUtf8();
    };

    QByteArray details = formatText(m_config.detailsText);
    QByteArray state = formatText(m_config.stateText);

    DiscordRichPresence discordPresence{
        .state = state,
        .details = details,
        .startTimestamp = m_config.showElapsedTime ? m_startTimestamp : 0,
        .endTimestamp = 0,
        .largeImageKey = "kate",
        .largeImageText = nullptr,
        .smallImageKey = nullptr,
        .smallImageText = nullptr,
        .partyId = nullptr,
        .partySize = 0,
        .partyMax = 0,
        .partyPrivacy = 0,
        .matchSecret = nullptr,
        .joinSecret = nullptr,
        .spectateSecret = nullptr,
        .instance = 0,
    };

    Discord_UpdatePresence(&discordPresence);
}

#include "discordrpcplugin.moc"
#include "moc_discordrpcplugin.cpp"
