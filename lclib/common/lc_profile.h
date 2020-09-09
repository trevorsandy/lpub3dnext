#pragma once

enum LC_PROFILE_KEY
{
	// Settings.
	LC_PROFILE_FIXED_AXES,
	LC_PROFILE_LINE_WIDTH,
	LC_PROFILE_ALLOW_LOD,
	LC_PROFILE_LOD_DISTANCE,
	LC_PROFILE_FADE_STEPS,
	LC_PROFILE_FADE_STEPS_COLOR,
	LC_PROFILE_HIGHLIGHT_NEW_PARTS,
	LC_PROFILE_HIGHLIGHT_NEW_PARTS_COLOR,
	LC_PROFILE_SHADING_MODE,
	LC_PROFILE_DRAW_AXES,
	LC_PROFILE_AXES_COLOR,
	LC_PROFILE_OVERLAY_COLOR,
	LC_PROFILE_ACTIVE_VIEW_COLOR,
	LC_PROFILE_DRAW_EDGE_LINES,
	LC_PROFILE_GRID_STUDS,
	LC_PROFILE_GRID_STUD_COLOR,
	LC_PROFILE_GRID_LINES,
	LC_PROFILE_GRID_LINE_SPACING,
	LC_PROFILE_GRID_LINE_COLOR,
	LC_PROFILE_ANTIALIASING_SAMPLES,
	LC_PROFILE_VIEW_SPHERE_ENABLED,
	LC_PROFILE_VIEW_SPHERE_LOCATION,
	LC_PROFILE_VIEW_SPHERE_SIZE,
	LC_PROFILE_VIEW_SPHERE_COLOR,
	LC_PROFILE_VIEW_SPHERE_TEXT_COLOR,
	LC_PROFILE_VIEW_SPHERE_HIGHLIGHT_COLOR,

	LC_PROFILE_LANGUAGE,
	LC_PROFILE_COLOR_THEME,
	LC_PROFILE_CHECK_UPDATES,
	LC_PROFILE_PROJECTS_PATH,
	LC_PROFILE_PARTS_LIBRARY,
	LC_PROFILE_PART_PALETTES,
	LC_PROFILE_MINIFIG_SETTINGS,
	LC_PROFILE_COLOR_CONFIG,
	LC_PROFILE_KEYBOARD_SHORTCUTS,
	LC_PROFILE_MOUSE_SHORTCUTS,
	LC_PROFILE_CATEGORIES,
	LC_PROFILE_RECENT_FILE1,
	LC_PROFILE_RECENT_FILE2,
	LC_PROFILE_RECENT_FILE3,
	LC_PROFILE_RECENT_FILE4,
	LC_PROFILE_AUTOLOAD_MOSTRECENT,
	LC_PROFILE_RESTORE_TAB_LAYOUT,
	LC_PROFILE_AUTOSAVE_INTERVAL,
	LC_PROFILE_MOUSE_SENSITIVITY,
	LC_PROFILE_IMAGE_WIDTH,
	LC_PROFILE_IMAGE_HEIGHT,
	LC_PROFILE_IMAGE_EXTENSION,
	LC_PROFILE_PARTS_LIST_ICONS,
	LC_PROFILE_PARTS_LIST_NAMES,
	LC_PROFILE_PARTS_LIST_COLOR,
	LC_PROFILE_PARTS_LIST_DECORATED,
	LC_PROFILE_PARTS_LIST_ALIASES,
	LC_PROFILE_PARTS_LIST_LISTMODE,
	LC_PROFILE_STUD_LOGO,

	// Defaults for new projects.
	LC_PROFILE_DEFAULT_AUTHOR_NAME,
	LC_PROFILE_DEFAULT_AMBIENT_COLOR,
	LC_PROFILE_DEFAULT_BACKGROUND_TYPE,
	LC_PROFILE_DEFAULT_BACKGROUND_COLOR,
	LC_PROFILE_DEFAULT_GRADIENT_COLOR1,
	LC_PROFILE_DEFAULT_GRADIENT_COLOR2,
	LC_PROFILE_DEFAULT_BACKGROUND_TEXTURE,
	LC_PROFILE_DEFAULT_BACKGROUND_TILE,

	// Exporters.
	LC_PROFILE_HTML_OPTIONS,
	LC_PROFILE_HTML_IMAGE_OPTIONS,
	LC_PROFILE_HTML_IMAGE_WIDTH,
	LC_PROFILE_HTML_IMAGE_HEIGHT,
	LC_PROFILE_POVRAY_PATH,
	LC_PROFILE_POVRAY_LGEO_PATH,
	LC_PROFILE_POVRAY_WIDTH,
	LC_PROFILE_POVRAY_HEIGHT,

/*** LPub3D Mod - Build Modification ***/
	LC_PROFILE_BUILD_MODIFICATION,
/*** LPub3D Mod end ***/

/*** LPub3D Mod - Camera Globe Target Position ***/
	LC_PROFILE_USE_IMAGE_SIZE,
	LC_PROFILE_AUTO_CENTER_SELECTION,
/*** LPub3D Mod end ***/

/*** LPub3D Mod - Update Default Camera ***/
	LC_PROFILE_DEFAULT_CAMERA_PROPERTIES,
	LC_PROFILE_DEFAULT_DISTANCE_FACTOR,
	LC_PROFILE_CAMERA_DEFAULT_POSITION,
	LC_PROFILE_CAMERA_FOV,
	LC_PROFILE_CAMERA_NEAR_PLANE,
	LC_PROFILE_CAMERA_FAR_PLANE,
/*** LPub3D Mod end ***/

/*** LPub3D Mod - Native Renderer settings ***/
	// Native Renderer.
	LC_PROFILE_NATIVE_VIEWPOINT,
	LC_PROFILE_NATIVE_PROJECTION,
/*** LPub3D Mod end ***/

/*** LPub3D Mod - Timeline part icons ***/
	LC_PROFILE_VIEW_PIECE_ICONS,
/*** LPub3D Mod end ***/

/*** LPub3D Mod - View point zoom extent ***/
	LC_PROFILE_VIEWPOINT_ZOOM_EXTENT,
/*** LPub3D Mod end ***/

/*** LPub3D Mod - true fade ***/
	LC_PROFILE_LPUB_TRUE_FADE,
	LC_PROFILE_CONDITIONAL_LINES,
/*** LPub3D Mod end ***/

/*** LPub3D Mod - preview widget ***/
	LC_PROFILE_ACTIVE_PREVIEW_COLOR,
	LC_PROFILE_VIEW_SPHERE_PREVIEW_SIZE,
	LC_PROFILE_VIEW_SPHERE_PREVIEW_LOCATION,
/*** LPub3D Mod end ***/

	LC_NUM_PROFILE_KEYS
};

enum LC_PROFILE_ENTRY_TYPE
{
	LC_PROFILE_ENTRY_INT,
	LC_PROFILE_ENTRY_FLOAT,
	LC_PROFILE_ENTRY_STRING,
	LC_PROFILE_ENTRY_STRINGLIST,
	LC_PROFILE_ENTRY_BUFFER
};

class lcProfileEntry
{
public:
	lcProfileEntry(const char* Section, const char* Key, int DefaultValue);
	lcProfileEntry(const char* Section, const char* Key, unsigned int DefaultValue);
	lcProfileEntry(const char* Section, const char* Key, float DefaultValue);
	lcProfileEntry(const char* Section, const char* Key, const char* DefaultValue);
	lcProfileEntry(const char* Section, const char* Key, const QStringList& StringList);
	lcProfileEntry(const char* Section, const char* Key);

	LC_PROFILE_ENTRY_TYPE mType;

	const char* mSection;
	const char* mKey;

	union
	{
		int IntValue;
		float FloatValue;
		const char* StringValue;
	} mDefault;
};

void lcRemoveProfileKey(LC_PROFILE_KEY Key);

int lcGetDefaultProfileInt(LC_PROFILE_KEY Key);
float lcGetDefaultProfileFloat(LC_PROFILE_KEY Key);
QString lcGetDefaultProfileString(LC_PROFILE_KEY Key);

int lcGetProfileInt(LC_PROFILE_KEY Key);
float lcGetProfileFloat(LC_PROFILE_KEY Key);
QString lcGetProfileString(LC_PROFILE_KEY Key);
QStringList lcGetProfileStringList(LC_PROFILE_KEY Key);
QByteArray lcGetProfileBuffer(LC_PROFILE_KEY Key);

void lcSetProfileInt(LC_PROFILE_KEY Key, int Value);
void lcSetProfileFloat(LC_PROFILE_KEY Key, float Value);
void lcSetProfileString(LC_PROFILE_KEY Key, const QString& Value);
void lcSetProfileStringList(LC_PROFILE_KEY Key, const QStringList& Value);
void lcSetProfileBuffer(LC_PROFILE_KEY Key, const QByteArray& Buffer);

