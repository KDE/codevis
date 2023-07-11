// clang-format off
#pragma once

// DO NOT EDIT THIS FILE
// This file was automatically generated by configuration-parser
// There will be a .conf file somewhere which was used to generate this file
// See https://github.com/tcanabrava/configuration-parser

#include <lakospreferences_export.h>
#include <functional>
#include <QObject>

#include <Qt>
#include <QFont>
#include <QApplication>
#include <QColor>


class LAKOSPREFERENCES_EXPORT Debug : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool enable_scene_context_menu READ enableSceneContextMenu WRITE setEnableSceneContextMenu NOTIFY enableSceneContextMenuChanged)
	Q_PROPERTY(bool enable_debug_output READ enableDebugOutput WRITE setEnableDebugOutput NOTIFY enableDebugOutputChanged)
	Q_PROPERTY(bool store_debug_output READ storeDebugOutput WRITE setStoreDebugOutput NOTIFY storeDebugOutputChanged)

public:
	Debug(QObject *parent = 0);
	 void loadDefaults();
	bool enableSceneContextMenu() const;
	void setEnableSceneContextMenuRule(std::function<bool(bool)> rule);
	bool enableSceneContextMenuDefault() const;
	bool enableDebugOutput() const;
	void setEnableDebugOutputRule(std::function<bool(bool)> rule);
	bool enableDebugOutputDefault() const;
	bool storeDebugOutput() const;
	void setStoreDebugOutputRule(std::function<bool(bool)> rule);
	bool storeDebugOutputDefault() const;

public:
	 Q_SLOT void setEnableSceneContextMenu(bool value);
	 Q_SLOT void setEnableDebugOutput(bool value);
	 Q_SLOT void setStoreDebugOutput(bool value);
	 Q_SIGNAL void enableSceneContextMenuChanged(bool value);
	 Q_SIGNAL void enableDebugOutputChanged(bool value);
	 Q_SIGNAL void storeDebugOutputChanged(bool value);

private:
	bool _enableSceneContextMenu;
	std::function<bool(bool)> enableSceneContextMenuRule;
	bool _enableDebugOutput;
	std::function<bool(bool)> enableDebugOutputRule;
	bool _storeDebugOutput;
	std::function<bool(bool)> storeDebugOutputRule;
};

class LAKOSPREFERENCES_EXPORT Document : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString last_document READ lastDocument WRITE setLastDocument NOTIFY lastDocumentChanged)
	Q_PROPERTY(bool use_dependency_types READ useDependencyTypes WRITE setUseDependencyTypes NOTIFY useDependencyTypesChanged)
	Q_PROPERTY(bool use_lakosian_rules READ useLakosianRules WRITE setUseLakosianRules NOTIFY useLakosianRulesChanged)
	Q_PROPERTY(int auto_save_backup_interval_msecs READ autoSaveBackupIntervalMsecs WRITE setAutoSaveBackupIntervalMsecs NOTIFY autoSaveBackupIntervalMsecsChanged)

public:
	Document(QObject *parent = 0);
	 void loadDefaults();
	QString lastDocument() const;
	void setLastDocumentRule(std::function<bool(QString)> rule);
	QString lastDocumentDefault() const;
	bool useDependencyTypes() const;
	void setUseDependencyTypesRule(std::function<bool(bool)> rule);
	bool useDependencyTypesDefault() const;
	bool useLakosianRules() const;
	void setUseLakosianRulesRule(std::function<bool(bool)> rule);
	bool useLakosianRulesDefault() const;
	int autoSaveBackupIntervalMsecs() const;
	void setAutoSaveBackupIntervalMsecsRule(std::function<bool(int)> rule);
	int autoSaveBackupIntervalMsecsDefault() const;

public:
	 Q_SLOT void setLastDocument(const QString& value);
	 Q_SLOT void setUseDependencyTypes(bool value);
	 Q_SLOT void setUseLakosianRules(bool value);
	 Q_SLOT void setAutoSaveBackupIntervalMsecs(int value);
	 Q_SIGNAL void lastDocumentChanged(const QString& value);
	 Q_SIGNAL void useDependencyTypesChanged(bool value);
	 Q_SIGNAL void useLakosianRulesChanged(bool value);
	 Q_SIGNAL void autoSaveBackupIntervalMsecsChanged(int value);

private:
	QString _lastDocument;
	std::function<bool(QString)> lastDocumentRule;
	bool _useDependencyTypes;
	std::function<bool(bool)> useDependencyTypesRule;
	bool _useLakosianRules;
	std::function<bool(bool)> useLakosianRulesRule;
	int _autoSaveBackupIntervalMsecs;
	std::function<bool(int)> autoSaveBackupIntervalMsecsRule;
};

class LAKOSPREFERENCES_EXPORT GraphTab : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool show_minimap READ showMinimap WRITE setShowMinimap NOTIFY showMinimapChanged)
	Q_PROPERTY(bool show_legend READ showLegend WRITE setShowLegend NOTIFY showLegendChanged)
	Q_PROPERTY(int class_limit READ classLimit WRITE setClassLimit NOTIFY classLimitChanged)
	Q_PROPERTY(int relation_limit READ relationLimit WRITE setRelationLimit NOTIFY relationLimitChanged)
	Q_PROPERTY(int zoom_level READ zoomLevel WRITE setZoomLevel NOTIFY zoomLevelChanged)
	Q_PROPERTY(int update_interval_msec READ updateIntervalMsec WRITE setUpdateIntervalMsec NOTIFY updateIntervalMsecChanged)

public:
	GraphTab(QObject *parent = 0);
	 void loadDefaults();
	bool showMinimap() const;
	void setShowMinimapRule(std::function<bool(bool)> rule);
	bool showMinimapDefault() const;
	bool showLegend() const;
	void setShowLegendRule(std::function<bool(bool)> rule);
	bool showLegendDefault() const;
	int classLimit() const;
	void setClassLimitRule(std::function<bool(int)> rule);
	int classLimitDefault() const;
	int relationLimit() const;
	void setRelationLimitRule(std::function<bool(int)> rule);
	int relationLimitDefault() const;
	int zoomLevel() const;
	void setZoomLevelRule(std::function<bool(int)> rule);
	int zoomLevelDefault() const;
	int updateIntervalMsec() const;
	void setUpdateIntervalMsecRule(std::function<bool(int)> rule);
	int updateIntervalMsecDefault() const;

public:
	 Q_SLOT void setShowMinimap(bool value);
	 Q_SLOT void setShowLegend(bool value);
	 Q_SLOT void setClassLimit(int value);
	 Q_SLOT void setRelationLimit(int value);
	 Q_SLOT void setZoomLevel(int value);
	 Q_SLOT void setUpdateIntervalMsec(int value);
	 Q_SIGNAL void showMinimapChanged(bool value);
	 Q_SIGNAL void showLegendChanged(bool value);
	 Q_SIGNAL void classLimitChanged(int value);
	 Q_SIGNAL void relationLimitChanged(int value);
	 Q_SIGNAL void zoomLevelChanged(int value);
	 Q_SIGNAL void updateIntervalMsecChanged(int value);

private:
	bool _showMinimap;
	std::function<bool(bool)> showMinimapRule;
	bool _showLegend;
	std::function<bool(bool)> showLegendRule;
	int _classLimit;
	std::function<bool(int)> classLimitRule;
	int _relationLimit;
	std::function<bool(int)> relationLimitRule;
	int _zoomLevel;
	std::function<bool(int)> zoomLevelRule;
	int _updateIntervalMsec;
	std::function<bool(int)> updateIntervalMsecRule;
};

class LAKOSPREFERENCES_EXPORT GraphWindow : public QObject {
	Q_OBJECT
	Q_PROPERTY(int drag_modifier READ dragModifier WRITE setDragModifier NOTIFY dragModifierChanged)
	Q_PROPERTY(int pan_modifier READ panModifier WRITE setPanModifier NOTIFY panModifierChanged)
	Q_PROPERTY(int zoom_modifier READ zoomModifier WRITE setZoomModifier NOTIFY zoomModifierChanged)
	Q_PROPERTY(int minimap_size READ minimapSize WRITE setMinimapSize NOTIFY minimapSizeChanged)
	Q_PROPERTY(bool color_blind_mode READ colorBlindMode WRITE setColorBlindMode NOTIFY colorBlindModeChanged)
	Q_PROPERTY(bool use_color_blind_fill READ useColorBlindFill WRITE setUseColorBlindFill NOTIFY useColorBlindFillChanged)
	Q_PROPERTY(QColor background_color READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
	Q_PROPERTY(QColor entity_background_color READ entityBackgroundColor WRITE setEntityBackgroundColor NOTIFY entityBackgroundColorChanged)
	Q_PROPERTY(QColor selected_entity_background_color READ selectedEntityBackgroundColor WRITE setSelectedEntityBackgroundColor NOTIFY selectedEntityBackgroundColorChanged)
	Q_PROPERTY(bool enable_gradient_on_main_node READ enableGradientOnMainNode WRITE setEnableGradientOnMainNode NOTIFY enableGradientOnMainNodeChanged)
	Q_PROPERTY(QColor edge_color READ edgeColor WRITE setEdgeColor NOTIFY edgeColorChanged)
	Q_PROPERTY(QColor highlight_edge_color READ highlightEdgeColor WRITE setHighlightEdgeColor NOTIFY highlightEdgeColorChanged)
	Q_PROPERTY(Qt::Corner lakos_entity_name_pos READ lakosEntityNamePos WRITE setLakosEntityNamePos NOTIFY lakosEntityNamePosChanged)
	Q_PROPERTY(bool show_redundant_edges_default READ showRedundantEdgesDefault WRITE setShowRedundantEdgesDefault NOTIFY showRedundantEdgesDefaultChanged)
	Q_PROPERTY(bool hide_package_prefix_on_components READ hidePackagePrefixOnComponents WRITE setHidePackagePrefixOnComponents NOTIFY hidePackagePrefixOnComponentsChanged)
	Q_PROPERTY(bool invert_horizontal_levelization_layout READ invertHorizontalLevelizationLayout WRITE setInvertHorizontalLevelizationLayout NOTIFY invertHorizontalLevelizationLayoutChanged)
	Q_PROPERTY(bool invert_vertical_levelization_layout READ invertVerticalLevelizationLayout WRITE setInvertVerticalLevelizationLayout NOTIFY invertVerticalLevelizationLayoutChanged)
	Q_PROPERTY(bool show_level_numbers READ showLevelNumbers WRITE setShowLevelNumbers NOTIFY showLevelNumbersChanged)

public:
	GraphWindow(QObject *parent = 0);
	 void loadDefaults();
	int dragModifier() const;
	void setDragModifierRule(std::function<bool(int)> rule);
	int dragModifierDefault() const;
	int panModifier() const;
	void setPanModifierRule(std::function<bool(int)> rule);
	int panModifierDefault() const;
	int zoomModifier() const;
	void setZoomModifierRule(std::function<bool(int)> rule);
	int zoomModifierDefault() const;
	int minimapSize() const;
	void setMinimapSizeRule(std::function<bool(int)> rule);
	int minimapSizeDefault() const;
	bool colorBlindMode() const;
	void setColorBlindModeRule(std::function<bool(bool)> rule);
	bool colorBlindModeDefault() const;
	bool useColorBlindFill() const;
	void setUseColorBlindFillRule(std::function<bool(bool)> rule);
	bool useColorBlindFillDefault() const;
	QColor backgroundColor() const;
	void setBackgroundColorRule(std::function<bool(QColor)> rule);
	QColor backgroundColorDefault() const;
	QColor entityBackgroundColor() const;
	void setEntityBackgroundColorRule(std::function<bool(QColor)> rule);
	QColor entityBackgroundColorDefault() const;
	QColor selectedEntityBackgroundColor() const;
	void setSelectedEntityBackgroundColorRule(std::function<bool(QColor)> rule);
	QColor selectedEntityBackgroundColorDefault() const;
	bool enableGradientOnMainNode() const;
	void setEnableGradientOnMainNodeRule(std::function<bool(bool)> rule);
	bool enableGradientOnMainNodeDefault() const;
	QColor edgeColor() const;
	void setEdgeColorRule(std::function<bool(QColor)> rule);
	QColor edgeColorDefault() const;
	QColor highlightEdgeColor() const;
	void setHighlightEdgeColorRule(std::function<bool(QColor)> rule);
	QColor highlightEdgeColorDefault() const;
	Qt::Corner lakosEntityNamePos() const;
	void setLakosEntityNamePosRule(std::function<bool(Qt::Corner)> rule);
	Qt::Corner lakosEntityNamePosDefault() const;
	bool showRedundantEdgesDefault() const;
	void setShowRedundantEdgesDefaultRule(std::function<bool(bool)> rule);
	bool showRedundantEdgesDefaultDefault() const;
	bool hidePackagePrefixOnComponents() const;
	void setHidePackagePrefixOnComponentsRule(std::function<bool(bool)> rule);
	bool hidePackagePrefixOnComponentsDefault() const;
	bool invertHorizontalLevelizationLayout() const;
	void setInvertHorizontalLevelizationLayoutRule(std::function<bool(bool)> rule);
	bool invertHorizontalLevelizationLayoutDefault() const;
	bool invertVerticalLevelizationLayout() const;
	void setInvertVerticalLevelizationLayoutRule(std::function<bool(bool)> rule);
	bool invertVerticalLevelizationLayoutDefault() const;
	bool showLevelNumbers() const;
	void setShowLevelNumbersRule(std::function<bool(bool)> rule);
	bool showLevelNumbersDefault() const;

public:
	 Q_SLOT void setDragModifier(int value);
	 Q_SLOT void setPanModifier(int value);
	 Q_SLOT void setZoomModifier(int value);
	 Q_SLOT void setMinimapSize(int value);
	 Q_SLOT void setColorBlindMode(bool value);
	 Q_SLOT void setUseColorBlindFill(bool value);
	 Q_SLOT void setBackgroundColor(const QColor& value);
	 Q_SLOT void setEntityBackgroundColor(const QColor& value);
	 Q_SLOT void setSelectedEntityBackgroundColor(const QColor& value);
	 Q_SLOT void setEnableGradientOnMainNode(bool value);
	 Q_SLOT void setEdgeColor(const QColor& value);
	 Q_SLOT void setHighlightEdgeColor(const QColor& value);
	 Q_SLOT void setLakosEntityNamePos(const Qt::Corner& value);
	 Q_SLOT void setShowRedundantEdgesDefault(bool value);
	 Q_SLOT void setHidePackagePrefixOnComponents(bool value);
	 Q_SLOT void setInvertHorizontalLevelizationLayout(bool value);
	 Q_SLOT void setInvertVerticalLevelizationLayout(bool value);
	 Q_SLOT void setShowLevelNumbers(bool value);
	 Q_SIGNAL void dragModifierChanged(int value);
	 Q_SIGNAL void panModifierChanged(int value);
	 Q_SIGNAL void zoomModifierChanged(int value);
	 Q_SIGNAL void minimapSizeChanged(int value);
	 Q_SIGNAL void colorBlindModeChanged(bool value);
	 Q_SIGNAL void useColorBlindFillChanged(bool value);
	 Q_SIGNAL void backgroundColorChanged(const QColor& value);
	 Q_SIGNAL void entityBackgroundColorChanged(const QColor& value);
	 Q_SIGNAL void selectedEntityBackgroundColorChanged(const QColor& value);
	 Q_SIGNAL void enableGradientOnMainNodeChanged(bool value);
	 Q_SIGNAL void edgeColorChanged(const QColor& value);
	 Q_SIGNAL void highlightEdgeColorChanged(const QColor& value);
	 Q_SIGNAL void lakosEntityNamePosChanged(const Qt::Corner& value);
	 Q_SIGNAL void showRedundantEdgesDefaultChanged(bool value);
	 Q_SIGNAL void hidePackagePrefixOnComponentsChanged(bool value);
	 Q_SIGNAL void invertHorizontalLevelizationLayoutChanged(bool value);
	 Q_SIGNAL void invertVerticalLevelizationLayoutChanged(bool value);
	 Q_SIGNAL void showLevelNumbersChanged(bool value);

private:
	int _dragModifier;
	std::function<bool(int)> dragModifierRule;
	int _panModifier;
	std::function<bool(int)> panModifierRule;
	int _zoomModifier;
	std::function<bool(int)> zoomModifierRule;
	int _minimapSize;
	std::function<bool(int)> minimapSizeRule;
	bool _colorBlindMode;
	std::function<bool(bool)> colorBlindModeRule;
	bool _useColorBlindFill;
	std::function<bool(bool)> useColorBlindFillRule;
	QColor _backgroundColor;
	std::function<bool(QColor)> backgroundColorRule;
	QColor _entityBackgroundColor;
	std::function<bool(QColor)> entityBackgroundColorRule;
	QColor _selectedEntityBackgroundColor;
	std::function<bool(QColor)> selectedEntityBackgroundColorRule;
	bool _enableGradientOnMainNode;
	std::function<bool(bool)> enableGradientOnMainNodeRule;
	QColor _edgeColor;
	std::function<bool(QColor)> edgeColorRule;
	QColor _highlightEdgeColor;
	std::function<bool(QColor)> highlightEdgeColorRule;
	Qt::Corner _lakosEntityNamePos;
	std::function<bool(Qt::Corner)> lakosEntityNamePosRule;
	bool _showRedundantEdgesDefault;
	std::function<bool(bool)> showRedundantEdgesDefaultRule;
	bool _hidePackagePrefixOnComponents;
	std::function<bool(bool)> hidePackagePrefixOnComponentsRule;
	bool _invertHorizontalLevelizationLayout;
	std::function<bool(bool)> invertHorizontalLevelizationLayoutRule;
	bool _invertVerticalLevelizationLayout;
	std::function<bool(bool)> invertVerticalLevelizationLayoutRule;
	bool _showLevelNumbers;
	std::function<bool(bool)> showLevelNumbersRule;
};

class LAKOSPREFERENCES_EXPORT Fonts : public QObject {
	Q_OBJECT
	Q_PROPERTY(QFont pkg_group_font READ pkgGroupFont WRITE setPkgGroupFont NOTIFY pkgGroupFontChanged)
	Q_PROPERTY(QFont pkg_font READ pkgFont WRITE setPkgFont NOTIFY pkgFontChanged)
	Q_PROPERTY(QFont component_font READ componentFont WRITE setComponentFont NOTIFY componentFontChanged)
	Q_PROPERTY(QFont class_font READ classFont WRITE setClassFont NOTIFY classFontChanged)
	Q_PROPERTY(QFont struct_font READ structFont WRITE setStructFont NOTIFY structFontChanged)
	Q_PROPERTY(QFont enum_font READ enumFont WRITE setEnumFont NOTIFY enumFontChanged)

public:
	Fonts(QObject *parent = 0);
	 void loadDefaults();
	QFont pkgGroupFont() const;
	void setPkgGroupFontRule(std::function<bool(QFont)> rule);
	QFont pkgGroupFontDefault() const;
	QFont pkgFont() const;
	void setPkgFontRule(std::function<bool(QFont)> rule);
	QFont pkgFontDefault() const;
	QFont componentFont() const;
	void setComponentFontRule(std::function<bool(QFont)> rule);
	QFont componentFontDefault() const;
	QFont classFont() const;
	void setClassFontRule(std::function<bool(QFont)> rule);
	QFont classFontDefault() const;
	QFont structFont() const;
	void setStructFontRule(std::function<bool(QFont)> rule);
	QFont structFontDefault() const;
	QFont enumFont() const;
	void setEnumFontRule(std::function<bool(QFont)> rule);
	QFont enumFontDefault() const;

public:
	 Q_SLOT void setPkgGroupFont(const QFont& value);
	 Q_SLOT void setPkgFont(const QFont& value);
	 Q_SLOT void setComponentFont(const QFont& value);
	 Q_SLOT void setClassFont(const QFont& value);
	 Q_SLOT void setStructFont(const QFont& value);
	 Q_SLOT void setEnumFont(const QFont& value);
	 Q_SIGNAL void pkgGroupFontChanged(const QFont& value);
	 Q_SIGNAL void pkgFontChanged(const QFont& value);
	 Q_SIGNAL void componentFontChanged(const QFont& value);
	 Q_SIGNAL void classFontChanged(const QFont& value);
	 Q_SIGNAL void structFontChanged(const QFont& value);
	 Q_SIGNAL void enumFontChanged(const QFont& value);

private:
	QFont _pkgGroupFont;
	std::function<bool(QFont)> pkgGroupFontRule;
	QFont _pkgFont;
	std::function<bool(QFont)> pkgFontRule;
	QFont _componentFont;
	std::function<bool(QFont)> componentFontRule;
	QFont _classFont;
	std::function<bool(QFont)> classFontRule;
	QFont _structFont;
	std::function<bool(QFont)> structFontRule;
	QFont _enumFont;
	std::function<bool(QFont)> enumFontRule;
};

class LAKOSPREFERENCES_EXPORT Tools : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool show_text READ showText WRITE setShowText NOTIFY showTextChanged)

public:
	Tools(QObject *parent = 0);
	 void loadDefaults();
	bool showText() const;
	void setShowTextRule(std::function<bool(bool)> rule);
	bool showTextDefault() const;

public:
	 Q_SLOT void setShowText(bool value);
	 Q_SIGNAL void showTextChanged(bool value);

private:
	bool _showText;
	std::function<bool(bool)> showTextRule;
};

class LAKOSPREFERENCES_EXPORT Window : public QObject {
	Q_OBJECT
Q_PROPERTY(QObject* graph_tab MEMBER _graphTab CONSTANT)
Q_PROPERTY(QObject* graph_window MEMBER _graphWindow CONSTANT)
Q_PROPERTY(QObject* fonts MEMBER _fonts CONSTANT)
Q_PROPERTY(QObject* tools MEMBER _tools CONSTANT)

public:
	Window(QObject *parent = 0);
	 void loadDefaults();
	GraphTab *graphTab() const;
	GraphWindow *graphWindow() const;
	Fonts *fonts() const;
	Tools *tools() const;

private:
	GraphTab *_graphTab;
	GraphWindow *_graphWindow;
	Fonts *_fonts;
	Tools *_tools;
};

class LAKOSPREFERENCES_EXPORT GraphLoadInfo : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool show_is_a_relation READ showIsARelation WRITE setShowIsARelation NOTIFY showIsARelationChanged)
	Q_PROPERTY(bool show_uses_in_the_implementation_relation READ showUsesInTheImplementationRelation WRITE setShowUsesInTheImplementationRelation NOTIFY showUsesInTheImplementationRelationChanged)
	Q_PROPERTY(bool show_uses_in_the_interface_relation READ showUsesInTheInterfaceRelation WRITE setShowUsesInTheInterfaceRelation NOTIFY showUsesInTheInterfaceRelationChanged)
	Q_PROPERTY(bool show_clients READ showClients WRITE setShowClients NOTIFY showClientsChanged)
	Q_PROPERTY(bool show_providers READ showProviders WRITE setShowProviders NOTIFY showProvidersChanged)
	Q_PROPERTY(bool show_external_edges READ showExternalEdges WRITE setShowExternalEdges NOTIFY showExternalEdgesChanged)

public:
	GraphLoadInfo(QObject *parent = 0);
	 void loadDefaults();
	bool showIsARelation() const;
	void setShowIsARelationRule(std::function<bool(bool)> rule);
	bool showIsARelationDefault() const;
	bool showUsesInTheImplementationRelation() const;
	void setShowUsesInTheImplementationRelationRule(std::function<bool(bool)> rule);
	bool showUsesInTheImplementationRelationDefault() const;
	bool showUsesInTheInterfaceRelation() const;
	void setShowUsesInTheInterfaceRelationRule(std::function<bool(bool)> rule);
	bool showUsesInTheInterfaceRelationDefault() const;
	bool showClients() const;
	void setShowClientsRule(std::function<bool(bool)> rule);
	bool showClientsDefault() const;
	bool showProviders() const;
	void setShowProvidersRule(std::function<bool(bool)> rule);
	bool showProvidersDefault() const;
	bool showExternalEdges() const;
	void setShowExternalEdgesRule(std::function<bool(bool)> rule);
	bool showExternalEdgesDefault() const;

public:
	 Q_SLOT void setShowIsARelation(bool value);
	 Q_SLOT void setShowUsesInTheImplementationRelation(bool value);
	 Q_SLOT void setShowUsesInTheInterfaceRelation(bool value);
	 Q_SLOT void setShowClients(bool value);
	 Q_SLOT void setShowProviders(bool value);
	 Q_SLOT void setShowExternalEdges(bool value);
	 Q_SIGNAL void showIsARelationChanged(bool value);
	 Q_SIGNAL void showUsesInTheImplementationRelationChanged(bool value);
	 Q_SIGNAL void showUsesInTheInterfaceRelationChanged(bool value);
	 Q_SIGNAL void showClientsChanged(bool value);
	 Q_SIGNAL void showProvidersChanged(bool value);
	 Q_SIGNAL void showExternalEdgesChanged(bool value);

private:
	bool _showIsARelation;
	std::function<bool(bool)> showIsARelationRule;
	bool _showUsesInTheImplementationRelation;
	std::function<bool(bool)> showUsesInTheImplementationRelationRule;
	bool _showUsesInTheInterfaceRelation;
	std::function<bool(bool)> showUsesInTheInterfaceRelationRule;
	bool _showClients;
	std::function<bool(bool)> showClientsRule;
	bool _showProviders;
	std::function<bool(bool)> showProvidersRule;
	bool _showExternalEdges;
	std::function<bool(bool)> showExternalEdgesRule;
};

class LAKOSPREFERENCES_EXPORT CodeExtractor : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString last_configure_json READ lastConfigureJson WRITE setLastConfigureJson NOTIFY lastConfigureJsonChanged)
	Q_PROPERTY(QString last_source_folder READ lastSourceFolder WRITE setLastSourceFolder NOTIFY lastSourceFolderChanged)
	Q_PROPERTY(QString last_ignore_pattern READ lastIgnorePattern WRITE setLastIgnorePattern NOTIFY lastIgnorePatternChanged)

public:
	CodeExtractor(QObject *parent = 0);
	 void loadDefaults();
	QString lastConfigureJson() const;
	void setLastConfigureJsonRule(std::function<bool(QString)> rule);
	QString lastConfigureJsonDefault() const;
	QString lastSourceFolder() const;
	void setLastSourceFolderRule(std::function<bool(QString)> rule);
	QString lastSourceFolderDefault() const;
	QString lastIgnorePattern() const;
	void setLastIgnorePatternRule(std::function<bool(QString)> rule);
	QString lastIgnorePatternDefault() const;

public:
	 Q_SLOT void setLastConfigureJson(const QString& value);
	 Q_SLOT void setLastSourceFolder(const QString& value);
	 Q_SLOT void setLastIgnorePattern(const QString& value);
	 Q_SIGNAL void lastConfigureJsonChanged(const QString& value);
	 Q_SIGNAL void lastSourceFolderChanged(const QString& value);
	 Q_SIGNAL void lastIgnorePatternChanged(const QString& value);

private:
	QString _lastConfigureJson;
	std::function<bool(QString)> lastConfigureJsonRule;
	QString _lastSourceFolder;
	std::function<bool(QString)> lastSourceFolderRule;
	QString _lastIgnorePattern;
	std::function<bool(QString)> lastIgnorePatternRule;
};

class LAKOSPREFERENCES_EXPORT CodeGeneration : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString last_output_dir READ lastOutputDir WRITE setLastOutputDir NOTIFY lastOutputDirChanged)

public:
	CodeGeneration(QObject *parent = 0);
	 void loadDefaults();
	QString lastOutputDir() const;
	void setLastOutputDirRule(std::function<bool(QString)> rule);
	QString lastOutputDirDefault() const;

public:
	 Q_SLOT void setLastOutputDir(const QString& value);
	 Q_SIGNAL void lastOutputDirChanged(const QString& value);

private:
	QString _lastOutputDir;
	std::function<bool(QString)> lastOutputDirRule;
};

class LAKOSPREFERENCES_EXPORT Preferences : public QObject {
	Q_OBJECT
Q_PROPERTY(QObject* debug MEMBER _debug CONSTANT)
Q_PROPERTY(QObject* document MEMBER _document CONSTANT)
Q_PROPERTY(QObject* window MEMBER _window CONSTANT)
Q_PROPERTY(QObject* graph_load_info MEMBER _graphLoadInfo CONSTANT)
Q_PROPERTY(QObject* code_extractor MEMBER _codeExtractor CONSTANT)
Q_PROPERTY(QObject* code_generation MEMBER _codeGeneration CONSTANT)

public:
	void sync();
	void load();
	static Preferences* self();
	 void loadDefaults();
	Debug *debug() const;
	Document *document() const;
	Window *window() const;
	GraphLoadInfo *graphLoadInfo() const;
	CodeExtractor *codeExtractor() const;
	CodeGeneration *codeGeneration() const;

private:
	Debug *_debug;
	Document *_document;
	Window *_window;
	GraphLoadInfo *_graphLoadInfo;
	CodeExtractor *_codeExtractor;
	CodeGeneration *_codeGeneration;
	Preferences(QObject *parent = 0);
};

// clang-format on
