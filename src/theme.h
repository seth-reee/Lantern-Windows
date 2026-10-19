#pragma once
#include <QObject>
#include <QGuiApplication>
#include <QPalette>
#include <QEvent>
#include <QVariantMap>

// Follow the platform palette, including live system appearance changes.
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap colors READ colors NOTIFY changed)
    Q_PROPERTY(QString iconSource READ iconSource NOTIFY changed)
public:
    Theme() {
        reload();
        qGuiApp->installEventFilter(this);
    }
    QVariantMap colors() const { return m_colors; }
    QString iconSource() const {
        return m_colors.value("background").value<QColor>().lightnessF() < 0.5
            ? "qrc:/resources/icons/lantern-black.png"
            : "qrc:/resources/icons/lantern-white.png";
    }
signals:
    void changed();
protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (event->type() == QEvent::ApplicationPaletteChange) reload();
        return QObject::eventFilter(object, event);
    }
private:
    void reload() {
        const QPalette palette = QGuiApplication::palette();
        const bool dark = palette.window().color().lightnessF() < 0.5;
        // Use a gentle system highlight tint so ordinary row text stays legible.
        const QColor base = palette.window().color();
        const QColor highlight = palette.highlight().color();
        const QColor selection = QColor::fromRgbF(
            base.redF() * 0.82 + highlight.redF() * 0.18,
            base.greenF() * 0.82 + highlight.greenF() * 0.18,
            base.blueF() * 0.82 + highlight.blueF() * 0.18);
        const QVariantMap colors = {
            {"background", base},
            {"lighter_background", palette.base().color()},
            {"foreground", palette.windowText().color()},
            {"accent", palette.accent().color()},
            {"selection", selection.name()},
            {"muted", palette.mid().color()},
            {"light_foreground", palette.placeholderText().color()},
            {"highlighted_text", palette.highlightedText().color()},
            {"green", dark ? QColor("#9ece6a") : QColor("#237a28")},
            {"red", dark ? QColor("#f7768e") : QColor("#b42332")},
            {"yellow", dark ? QColor("#e0af68") : QColor("#805b00")}
        };
        if (m_colors == colors) return;
        m_colors = colors;
        emit changed();
    }
    QVariantMap m_colors;
};
