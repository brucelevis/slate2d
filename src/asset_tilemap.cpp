#include <assert.h>
#include "assetloader.h"
#include "files.h"
#include <tmx.h>
#include "main.h"

void * tmx_img_load(const char *path) {
	const char *fullpath = tempstr("maps/%s", path);
	AssetHandle handle = Asset_Create(ASSET_IMAGE, fullpath, fullpath);
	return (void*)Asset_Get(ASSET_IMAGE, handle);
}

void tmx_img_free(void *address) {
	NOTUSED(address);
	// leave the asset around, since they'll often be cleared by the next scene load
}

void *tmx_fs(const char *filename, int *outSz) {
	void *xml;

	*outSz = FS_ReadFile(tempstr("maps/%s", filename), &xml);

	if (*outSz < 0) {
		Con_Errorf(ERR_GAME, "Couldn't load file while parsing map %s", filename);
		return nullptr;
	}

	return xml;
}

void * TMX_Load(Asset &asset) {
	tmx_img_load_func = &tmx_img_load;
	tmx_img_free_func = &tmx_img_free;
	tmx_file_read_func = &tmx_fs;

	tmx_map *map;

	const char *xml;
	int outSz = FS_ReadFile(asset.path, (void **)&xml);
	if (outSz < 0) {
		Con_Errorf(ERR_GAME, "Couldn't read map %s", asset.path);
		return nullptr;
	}

	map = tmx_load_buffer(xml, outSz);

	if (map == nullptr) {
		Con_Errorf(ERR_GAME, "Failed to load tmx %s", asset.path);
		free((void*)xml);
		return nullptr;
	}

	if (map->orient != O_ORT) {
		Con_Errorf(ERR_GAME, "Non orthagonal tiles not supported in tmx %s", asset.path);
		free((void*)xml);
		return nullptr;
	}

	free((void*)xml);

	return (void*) map;
}

void TMX_Free(Asset &asset) {
	tmx_map_free((tmx_map*) asset.resource);
}

tmx_map* Get_TMX(AssetHandle id) {
	Asset *asset = Asset_Get(ASSET_TMX, id);
	assert(asset != nullptr && asset->resource != nullptr);
	return (tmx_map*)asset->resource;
}