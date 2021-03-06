#ifndef DLGAUTODJ_H
#define DLGAUTODJ_H

#include <QWidget>
#include <QString>

#include "library/autodj/ui_dlgautodj.h"
#include "preferences/usersettings.h"
#include "track/track.h"
#include "library/libraryview.h"
#include "library/library.h"
#include "library/trackcollection.h"
#include "library/autodj/autodjprocessor.h"
#include "controllers/keyboard/keyboardeventfilter.h"

class PlaylistTableModel;
class WTrackTableView;

class DlgAutoDJ : public QWidget, public Ui::DlgAutoDJ, public LibraryView {
    Q_OBJECT
  public:
    DlgAutoDJ(QWidget* parent,
            UserSettingsPointer pConfig,
            Library* pLibrary,
            AutoDJProcessor* pProcessor,
            TrackCollection* pTrackCollection,
            KeyboardEventFilter* pKeyboard,
            bool showButtonText);
    ~DlgAutoDJ() override;

    void onShow() override;
    bool hasFocus() const override;
    void onSearch(const QString& text) override;
    void loadSelectedTrack() override;
    void loadSelectedTrackToGroup(QString group, bool play) override;
    void moveSelection(int delta) override;

  public slots:
    void shufflePlaylistButton(bool buttonChecked);
    void skipNextButton(bool buttonChecked);
    void fadeNowButton(bool buttonChecked);
    void toggleAutoDJButton(bool enable);
    void transitionTimeChanged(int time);
    void transitionSliderChanged(int value);
    void autoDJStateChanged(AutoDJProcessor::AutoDJState state);
    void updateSelectionInfo();
    void slotTransitionModeChanged(int comboboxIndex);
    void slotRepeatPlaylistChanged(int checkedState);

  signals:
    void addRandomButton(bool buttonChecked);
    void loadTrack(TrackPointer tio);
    void loadTrackToPlayer(TrackPointer tio, QString group, bool);
    void trackSelected(TrackPointer pTrack);

  private:
    void setupActionButton(QPushButton* pButton,
            void (DlgAutoDJ::*pSlot)(bool),
            QString fallbackText);

    AutoDJProcessor* m_pAutoDJProcessor;
    WTrackTableView* m_pTrackTableView;
    PlaylistTableModel* m_pAutoDJTableModel;
    UserSettingsPointer m_pConfig;
    bool m_bShowButtonText;
};

#endif //DLGAUTODJ_H
