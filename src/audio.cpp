#include "assetloader.h"
#include "files.h"
#include "../game/shared.h"

#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_thread.h>
#include <soloud_speech.h>
#include <soloud_modplug.h>

extern SoLoud::Soloud soloud;

void* Speech_Load(Asset &asset) {
	auto speech = new SoLoud::Speech();
	speech->setText(asset.path);

	return (void*)speech;
}

void Speech_Free(Asset &asset) {
	SoLoud::Speech* speech = (SoLoud::Speech*) asset.resource;
	delete speech;
}

void* Sound_Load(Asset &asset) {
	unsigned char *musicbuf;
	auto sz = FS_ReadFile(asset.path, (void **)&musicbuf);

	if (sz <= 0) {
		return nullptr;
	}

	switch (asset.type) {
		case ASSET_SOUND: {
			auto sound = new SoLoud::Wav();
			sound->loadMem(musicbuf, sz, false, true);
			return (void*)sound;
		}

		case ASSET_MOD: {
			auto mod = new SoLoud::Modplug();
			mod->loadMem(musicbuf, sz, false, true);
			return (void*)mod;
		}
	}

	return nullptr;
}

void Sound_Free(Asset &asset) {
	SoLoud::Wav* sound = (SoLoud::Wav*) asset.resource;
	delete sound;
}

void Mod_Free(Asset &asset) {
	SoLoud::Modplug* mod = (SoLoud::Modplug*) asset.resource;
	delete mod;
}

void Snd_Play(AssetHandle assetHandle, float volume, float pan, bool loop) {
	auto asset = Asset_Get(assetHandle);
	if (asset->type != ASSET_SOUND && asset->type != ASSET_SPEECH && asset->type != ASSET_MOD) {
		return;
	}

	SoLoud::AudioSource *src = (SoLoud::AudioSource*) asset->resource;
	src->setLooping(loop);
	soloud.play(*src, volume, pan);
}