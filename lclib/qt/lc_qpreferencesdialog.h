#pragma once

#include <QDialog>
struct lcPreferencesDialogOptions;

namespace Ui {
class lcQPreferencesDialog;
}

class lcQPreferencesDialog : public QDialog
{
	Q_OBJECT
	
public:
	lcQPreferencesDialog(QWidget* Parent, lcPreferencesDialogOptions* Options);
	~lcQPreferencesDialog();

	lcPreferencesDialogOptions* mOptions;

	enum
	{
		CategoryRole = Qt::UserRole
	};

	bool eventFilter(QObject* Object, QEvent* Event) override;

public slots:
	void accept() override;
	void on_partsLibraryBrowse_clicked();
	void on_partsArchiveBrowse_clicked();
	void on_ColorConfigBrowseButton_clicked();
	void on_MinifigSettingsBrowseButton_clicked();
	void on_povrayExecutableBrowse_clicked();
	void on_lgeoPathBrowse_clicked();
/*** LPub3D Mod - Suppress compatible signals warning ***/
	void on_ColorTheme_currentIndexChanged(int Index);
/*** LPub3D Mod end ***/
	void ColorButtonClicked();
	void on_antiAliasing_toggled();
	void on_edgeLines_toggled();
	void on_LineWidthSlider_valueChanged();
	void on_FadeSteps_toggled();
	void on_HighlightNewParts_toggled();
	void on_gridStuds_toggled();
	void on_gridLines_toggled();
	void on_ViewSphereSizeCombo_currentIndexChanged(int Index);
	void updateParts();
	void on_newCategory_clicked();
	void on_editCategory_clicked();
	void on_deleteCategory_clicked();
	void on_importCategories_clicked();
	void on_exportCategories_clicked();
	void on_resetCategories_clicked();
	void on_shortcutAssign_clicked();
	void on_shortcutRemove_clicked();
	void on_shortcutsImport_clicked();
	void on_shortcutsExport_clicked();
	void on_shortcutsReset_clicked();
	void commandChanged(QTreeWidgetItem *current);
	void on_KeyboardFilterEdit_textEdited(const QString& Text);
	void on_mouseAssign_clicked();
	void on_mouseRemove_clicked();
	void on_mouseReset_clicked();
	void on_studLogo_toggled();
	void MouseTreeItemChanged(QTreeWidgetItem* Current);

/*** LPub3D Mod - Native Renderer settings ***/
	void on_ViewpointsCombo_currentIndexChanged(int index);
/*** LPub3D Mod end ***/
/*** LPub3D Mod - Reset theme colors ***/
	void on_ResetColorsButton_clicked();
/*** LPub3D Mod end ***/
/*** LPub3D Mod - Update Default Camera ***/
	void on_cameraDefaultDistanceFactor_valueChanged(double value);
	void on_cameraDefaultPosition_valueChanged(double value);
	void cameraPropertyReset();
/*** LPub3D Mod end ***/

private:
	Ui::lcQPreferencesDialog *ui;

	void updateCategories();
	void updateCommandList();
	void UpdateMouseTree();
	void UpdateMouseTreeItem(int ItemIndex);
	void setShortcutModified(QTreeWidgetItem *treeItem, bool modified);

	float mLineWidthRange[2];
	float mLineWidthGranularity;
};
