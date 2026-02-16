#ifndef DISCORDRPCPLUGIN_H
#define DISCORDRPCPLUGIN_H

#include <KTextEditor/Plugin>
#include <QTimer>

class DiscordRpcPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit DiscordRpcPlugin(QObject *parent = nullptr);
    ~DiscordRpcPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    [[nodiscard]] int configPages() const override
    {
        return 1;
    }
    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

    struct RPCConfig {
        QString detailsText;
        QString stateText;
        bool showElapsedTime;
    };

    void readConfig();

    void updateStatus() const;

    [[nodiscard]] RPCConfig &config()
    {
        return m_config;
    }
    static const RPCConfig DEFAULT_CONFIG;

private:
    void initDiscord();

    int64_t m_startTimestamp;
    QTimer m_updateTimer;

    RPCConfig m_config{};
};

#endif
