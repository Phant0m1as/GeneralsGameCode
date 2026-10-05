/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: MapUtil.h /////////////////////////////////////////////////////////
// Author: Matt Campbell, December 2001
// Description: Map utility/convenience routines
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/AsciiString.h"
#include "Common/UnicodeString.h"

#include "Common/STLTypedefs.h"

class GameWindow;
class AsciiString;
struct Coord3D;
struct FileInfo;
class Image;
class DataChunkInput;
struct DataChunkInfo;
// This matches the windows timestamp.
enum { SUPPLY_TECH_SIZE = 15};
typedef std::list <ICoord2D> ICoord2DList;

class TechAndSupplyImages
{
public:
	ICoord2DList m_techPosList;
	ICoord2DList m_supplyPosList;
};

struct WinTimeStamp
{
	UnsignedInt m_lowTimeStamp;
	UnsignedInt m_highTimeStamp;
};


class WaypointMap : public std::map<AsciiString, Coord3D>
{
public:
	void update();	///< returns the number of multiplayer start spots found
	Int m_numStartSpots;
};

typedef std::list <Coord3D> Coord3DList;

class MapMetaData
{
public:
	UnicodeString m_displayName;
	AsciiString m_nameLookupTag;
	Region3D m_extent;
	Int m_numPlayers;

	Bool m_isMultiplayer;
	Bool m_isOfficial;
	Bool m_doesExist; ///< Flag to indicate whether the map physically exists. Should be true.
	UnsignedInt m_filesize;
	UnsignedInt m_CRC;

	WinTimeStamp m_timestamp;

	WaypointMap m_waypoints;
	Coord3DList m_supplyPositions;
	Coord3DList m_techPositions;
	AsciiString m_fileName;
};

// TheSuperHackers @performance xezon 02/11/2025 Simplifies and improves the implementation of MapCache
// to prevent expensive reoccurring redundant map cache reads.

class MapCache : public std::map<AsciiString, MapMetaData>
{
	typedef std::set<AsciiString> MapNameSet;

public:
	MapCache()
		: m_doCreateStandardMapCacheINI(TRUE)
		, m_doLoadStandardMapCacheINI(TRUE)
		, m_doLoadUserMapCacheINI(TRUE)
	{}

	void updateCache();

	AsciiString getMapDir() const;
	AsciiString getUserMapDir() const;
	AsciiString getMapExtension() const;

	const MapMetaData *findMap(AsciiString mapName);

	// allow us to create a set of shippable maps to be in mapcache.ini.  For use with -buildMapCache.
	void addShippingMap(AsciiString mapName) { mapName.toLower(); m_allowedMaps.insert(mapName); }

private:
	void prepareUnseenMaps(const AsciiString &mapDir);
	Bool clearUnseenMaps(const AsciiString &mapDir);
	void loadMapsFromMapCacheINI(const AsciiString &mapDir);
	Bool loadMapsFromDisk(const AsciiString &mapDir, Bool isOfficial, Bool filterByAllowedMaps = FALSE); // returns true if we needed to (re)parse a map
	Bool addMap(const AsciiString &mapDir, const AsciiString &fname, const AsciiString &lowerFname, FileInfo &fileInfo, Bool isOfficial); ///< returns true if it had to (re)parse the map
	void writeCacheINI(const AsciiString &mapDir);

	static const char *const m_mapCacheName;

	MapNameSet m_allowedMaps;
	Bool m_doCreateStandardMapCacheINI;
	Bool m_doLoadStandardMapCacheINI;
	Bool m_doLoadUserMapCacheINI;
};

extern MapCache *TheMapCache;
extern TechAndSupplyImages TheSupplyAndTechImageLocations;

// TheSuperHackers @refactor xezon 28/11/2025 Refactors the map list population implementation
// by breaking it into smaller pieces to make it more maintainable.

Int populateMapListbox( GameWindow *listbox, Bool useSystemMaps, Bool isMultiplayer, AsciiString mapToSelect = AsciiString::TheEmptyString );		/// Read a list of maps from the run directory and fill in the listbox.  Return the selected index
Int populateMapListboxNoReset( GameWindow *listbox, Bool useSystemMaps, Bool isMultiplayer, AsciiString mapToSelect = AsciiString::TheEmptyString );		/// Read a list of maps from the run directory and fill in the listbox.  Return the selected index
Bool isValidMap( AsciiString mapName, Bool isMultiplayer );						/// Validate a map
Image *getMapPreviewImage( AsciiString mapName );
AsciiString getDefaultMap( Bool isMultiplayer );											/// Find a valid map
AsciiString getDefaultOfficialMap();
Bool isOfficialMap( AsciiString mapName );
Bool parseMapPreviewChunk(DataChunkInput &file, DataChunkInfo *info, void *userData);
void findDrawPositions( Int startX, Int startY, Int width, Int height, Region3D extent,
															 ICoord2D *ul, ICoord2D *lr );
Bool WouldMapTransfer( const AsciiString& mapName );

// Map favorites: the player can mark maps as favorites in the map selection menus and filter the
// map list to only show favorites. Favorites are stored locally in "MapFavorites.ini" in the user
// data directory. Favorites of maps that no longer exist are simply never shown.
Bool isMapFavorite( const AsciiString& mapName );
void setMapFavorite( const AsciiString& mapName, Bool favorite );
Bool getMapFavoritesFilter();												///< TRUE if the map lists only show favorites
void setMapFavoritesFilter( Bool favoritesOnly );

// Map search: filters the map lists by a part of the map name (case insensitive).
// The search text is not stored and is cleared every time a map selection menu is opened.
Bool isMapSearchActive();

/// Creates and handles the map list tools of a map selection menu: a search field with a clear
/// button above the map listbox, and the two favorites buttons below it.
class MapFavoritesButtons
{
public:
	/// Called to refill the map listbox. mapToSelect is the map that should stay selected, if it is
	/// still in the list. Otherwise nothing is selected.
	typedef void (*RepopulateFunc)( const AsciiString& mapToSelect );

	MapFavoritesButtons();

	/// Creates the search field above and the favorites buttons below the map listbox. The listbox
	/// is made smaller to make room for them. The buttons copy the look of templateButton.
	/// Clears the search text. Call before populating the map listbox.
	/// The label in front of the search field uses the font of labelFontTemplate, if given.
	void create( GameWindow *mapListbox, GameWindow *templateButton, RepopulateFunc repopulate,
		GameWindow *labelFontTemplate = nullptr );

	/// Forgets the window pointers. The windows themselves are destroyed together with the layout.
	void clear();

	/// Handles a click on one of the map list tool buttons. Repopulates the map listbox if needed.
	/// Returns TRUE if the control was one of the map list tool buttons.
	Bool onButtonSelected( const GameWindow *control );

	/// Handles a change of the search text. Repopulates the map listbox.
	/// Returns TRUE if the control was the search field.
	Bool onTextChanged( const GameWindow *control );

	/// Updates the label of the favorite button for the currently selected map.
	void updateFromSelection();

	/// Returns the file name of the currently selected map, or an empty string.
	AsciiString getSelectedMap() const;

	/// Returns the map the player chose last. It stays chosen while it is hidden by the search or
	/// favorites filter, until the player clicks another map. Empty if no map was chosen.
	AsciiString getChosenMap() const;

private:
	void updateFilterButtonText();
	void repopulate();

	GameWindow *m_listbox;
	GameWindow *m_buttonFavorite;
	GameWindow *m_buttonFilter;
	GameWindow *m_searchEntry;
	GameWindow *m_buttonClearSearch;
	RepopulateFunc m_repopulate;

	// A plain array is used on purpose, because the menus keep this object in a static variable.
	char m_chosenMap[520];
};
