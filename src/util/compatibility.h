#ifndef COMPATABILITY_H
#define COMPATABILITY_H

#include <QCoreApplication>
#include <QGuiApplication>
#include <QList>
#include <QScreen>
#include <QWindow>
#include <QWidget>

#include "util/assert.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)

// this adds const to non-const objects (like std::as_const)
template <typename T>
struct QAddConst { typedef const T Type; };
template <typename T>
constexpr typename QAddConst<T>::Type &qAsConst(T &t) { return t; }
// prevent rvalue arguments:
template <typename T>
void qAsConst(const T &&) = delete;

template <typename... Args>
struct QNonConstOverload
{
    template <typename R, typename T>
    Q_DECL_CONSTEXPR auto operator()(R (T::*ptr)(Args...)) const Q_DECL_NOTHROW -> decltype(ptr)
    { return ptr; }

    template <typename R, typename T>
    static Q_DECL_CONSTEXPR auto of(R (T::*ptr)(Args...)) Q_DECL_NOTHROW -> decltype(ptr)
    { return ptr; }
};

template <typename... Args>
struct QConstOverload
{
    template <typename R, typename T>
    Q_DECL_CONSTEXPR auto operator()(R (T::*ptr)(Args...) const) const Q_DECL_NOTHROW -> decltype(ptr)
    { return ptr; }

    template <typename R, typename T>
    static Q_DECL_CONSTEXPR auto of(R (T::*ptr)(Args...) const) Q_DECL_NOTHROW -> decltype(ptr)
    { return ptr; }
};

template <typename... Args>
struct QOverload : QConstOverload<Args...>, QNonConstOverload<Args...>
{
    using QConstOverload<Args...>::of;
    using QConstOverload<Args...>::operator();
    using QNonConstOverload<Args...>::of;
    using QNonConstOverload<Args...>::operator();

    template <typename R>
    Q_DECL_CONSTEXPR auto operator()(R (*ptr)(Args...)) const Q_DECL_NOTHROW -> decltype(ptr)
    { return ptr; }

    template <typename R>
    static Q_DECL_CONSTEXPR auto of(R (*ptr)(Args...)) Q_DECL_NOTHROW -> decltype(ptr)
    { return ptr; }
};

#endif


inline qreal getDevicePixelRatioF(const QWidget* widget) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    return widget->devicePixelRatioF();
#endif

    // Crawl up to the window and return qreal value
    QWindow* window = widget->window()->windowHandle();
    if (window) {
        return window->devicePixelRatio();
    }

    // return integer value as last resort
    return widget->devicePixelRatio();
}

inline QScreen* getPrimaryScreen() {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QGuiApplication* app = static_cast<QGuiApplication*>(QCoreApplication::instance());
    VERIFY_OR_DEBUG_ASSERT(app) {
        qWarning() << "Unable to get applications QCoreApplication instance, cannot determine primary screen!";
    } else {
        return app->primaryScreen();
    }
#endif
    const QList<QScreen*> screens = QGuiApplication::screens();
    VERIFY_OR_DEBUG_ASSERT(!screens.isEmpty()) {
        qWarning() << "No screens found, cannot determine primary screen!";
    } else {
        return screens.first();
    }

    // All attempts to find primary screen failed, return nullptr
    return nullptr;
}

#endif /* COMPATABILITY_H */
