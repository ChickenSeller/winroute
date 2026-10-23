#pragma once

#include "core/Profile.h"

#include <QDialog>
#include <QTreeWidget>
#include <functional>

class QLabel;
class QPushButton;

// Chain tree that accepts proxies dragged from the proxy list and lets the
// members of a chain be reordered by dragging.
class ChainTree : public QTreeWidget {
    Q_OBJECT
public:
    explicit ChainTree(QWidget *parent = nullptr);
    QTreeWidget *proxySource = nullptr;
    std::function<void(int chainId, int proxyId, int index)> onAddProxy;
    std::function<void(int chainId, int fromIndex, int toIndex)> onMoveMember;

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dropEvent(QDropEvent *e) override;
};

// "Proxy Servers" dialog with the "Proxy Chains" section.
class ProxyServersDialog : public QDialog {
    Q_OBJECT
public:
    ProxyServersDialog(const Profile &profile, QWidget *parent = nullptr);
    Profile profile() const { return m_profile; }
    void showChains(bool show);

signals:
    void resetFailuresRequested();

private:
    void rebuildProxies(int selectId = 0);
    void rebuildChains(int selectChainId = 0);
    int selectedProxyId() const;
    int selectedChainId() const;
    void addProxy();
    void editProxy();
    void removeProxy();
    void checkProxy();
    void createChain();
    void chainType();
    void removeChainOrMember();
    // Asks how to handle rules that use the proxy/chain about to be removed. Returns false on cancel.
    bool resolveRemoval(ActionType type, int id, const QString &name);

    Profile m_profile;
    QTreeWidget *m_proxies;
    ChainTree *m_chains;
    QWidget *m_chainPanel;
    QLabel *m_chainLabel;
    QPushButton *m_chainsButton;
};
