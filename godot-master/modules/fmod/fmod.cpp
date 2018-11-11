#include "fmod.h"

Implementation::Implementation() {
    mpStudioSystem = NULL;
    FMod::ErrorCheck(FMOD::Studio::System::create(&mpStudioSystem));
    FMod::ErrorCheck(mpStudioSystem->initialize(32, FMOD_STUDIO_INIT_LIVEUPDATE, FMOD_INIT_PROFILE_ENABLE, NULL));

    mpSystem = NULL;
    FMod::ErrorCheck(mpStudioSystem->getLowLevelSystem(&mpSystem));
}

Implementation::~Implementation() {
    FMod::ErrorCheck(mpStudioSystem->unloadAll());
    FMod::ErrorCheck(mpStudioSystem->release());
}

void Implementation::Update() {
    vector<ChannelMap::iterator> pStoppedChannels;
    for (auto it = mChannels.begin(), itEnd = mChannels.end(); it != itEnd; ++it)
    {
        bool bIsPlaying = false;
        it->second->isPlaying(&bIsPlaying);
        if (!bIsPlaying)
        {
             pStoppedChannels.push_back(it);
        }
    }
    for (auto& it : pStoppedChannels)
    {
         mChannels.erase(it);
    }
    FMod::ErrorCheck(mpStudioSystem->update());
}

Implementation* sgpImplementation = nullptr;

FMOD_VECTOR FMod::VectorToFmod(const Vector3& vPosition){
	FMOD_VECTOR fVec;
	fVec.x = vPosition.x;
	fVec.y = vPosition.y;
	fVec.z = vPosition.z;
	return fVec;
}

float  FMod::dbToVolume(float dB)
{
	return powf(10.0f, 0.05f * dB);
}

float  FMod::VolumeTodb(float volume)
{
	return 20.0f * log10f(volume);
}


void FMod::Init() {
    sgpImplementation = new Implementation;
}

void FMod::Update() {
    sgpImplementation->Update();
}

void FMod::LoadSound(const std::string& strSoundName, bool b3d, bool bLooping, bool bStream)
{
	auto tFoundIt = sgpImplementation->mSounds.find(strSoundName);
	if (tFoundIt != sgpImplementation->mSounds.end())
		return;
	FMOD_MODE eMode = FMOD_DEFAULT;
	eMode |= b3d ? FMOD_3D : FMOD_2D; //sono flag per fmod su come caricare il suono (2d o 3d, sample o stream ) 
	eMode |= bLooping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
	eMode |= bStream ? FMOD_CREATESTREAM : FMOD_CREATECOMPRESSEDSAMPLE;
	FMOD::Sound* pSound = nullptr;
	FMod::ErrorCheck(sgpImplementation->mpSystem->createSound(strSoundName.c_str(), eMode, nullptr, &pSound));
	if (pSound){
		sgpImplementation->mSounds[strSoundName] = pSound;
	}
}

void FMod::UnLoadSound(const std::string& strSoundName)
{
	auto tFoundIt = sgpImplementation->mSounds.find(strSoundName);
	if (tFoundIt == sgpImplementation->mSounds.end())
		return;
	FMod::ErrorCheck(tFoundIt->second->release());
	sgpImplementation->mSounds.erase(tFoundIt);
}

int FMod::PlaySound(const string& strSoundName, const Vector3& vPosition, float fVolumedB)
{
	int nChannelId = sgpImplementation->mnNextChannelId++;
	auto tFoundIt = sgpImplementation->mSounds.find(strSoundName);
	if (tFoundIt == sgpImplementation->mSounds.end())
	{
		//LoadSound(strSoundName); TODO
		return 0;
		tFoundIt = sgpImplementation->mSounds.find(strSoundName);
		if (tFoundIt == sgpImplementation->mSounds.end())
		{
			return nChannelId;
		}
	}
	FMOD::Channel* pChannel = nullptr;
	FMod::ErrorCheck(sgpImplementation->mpSystem->playSound(tFoundIt->second, nullptr, true, &pChannel));
	if (pChannel)
	{
		FMOD_MODE currMode;
		tFoundIt->second->getMode(&currMode);
		if (currMode & FMOD_3D){
			FMOD_VECTOR position = VectorToFmod(vPosition);
			FMod::ErrorCheck(pChannel->set3DAttributes(&position, nullptr));
		}
		FMod::ErrorCheck(pChannel->setVolume(dbToVolume(fVolumedB)));
		FMod::ErrorCheck(pChannel->setPaused(false));
		sgpImplementation->mChannels[nChannelId] = pChannel;
	}
	return nChannelId;
}

void FMod::SetChannel3dPosition(int nChannelId, const Vector3& vPosition)
{
	auto tFoundIt = sgpImplementation->mChannels.find(nChannelId);
	if (tFoundIt == sgpImplementation->mChannels.end())
		return;

	FMOD_VECTOR position = VectorToFmod(vPosition);
	FMod::ErrorCheck(tFoundIt->second->set3DAttributes(&position, NULL));
}

void FMod::SetChannelVolume(int nChannelId, float fVolumedB)
{
	auto tFoundIt = sgpImplementation->mChannels.find(nChannelId);
	if (tFoundIt == sgpImplementation->mChannels.end())
		return;

	FMod::ErrorCheck(tFoundIt->second->setVolume(dbToVolume(fVolumedB)));
}

void FMod::LoadBank(const std::string& strBankName, FMOD_STUDIO_LOAD_BANK_FLAGS flags) {
	auto tFoundIt = sgpImplementation->mBanks.find(strBankName);
	if (tFoundIt != sgpImplementation->mBanks.end())
		return;
	FMOD::Studio::Bank* pBank;
	FMod::ErrorCheck(sgpImplementation->mpStudioSystem->loadBankFile(strBankName.c_str(), flags, &pBank));
	if (pBank) {
		sgpImplementation->mBanks[strBankName] = pBank;
	}
}

void FMod::LoadEvent(const std::string& strEventName) {
	auto tFoundit = sgpImplementation->mEvents.find(strEventName);
	if (tFoundit != sgpImplementation->mEvents.end())
		return;
	FMOD::Studio::EventDescription* pEventDescription = NULL;
	FMod::ErrorCheck(sgpImplementation->mpStudioSystem->getEvent(strEventName.c_str(), &pEventDescription));
	if (pEventDescription){
		FMOD::Studio::EventInstance* pEventInstance = NULL;
		FMod::ErrorCheck(pEventDescription->createInstance(&pEventInstance));
		if (pEventInstance){
			sgpImplementation->mEvents[strEventName] = pEventInstance;
		}
	}	
}

void FMod::PlayEvent(const string &strEventName) {
	auto tFoundit = sgpImplementation->mEvents.find(strEventName);
	if (tFoundit == sgpImplementation->mEvents.end()){
		LoadEvent(strEventName);
		tFoundit = sgpImplementation->mEvents.find(strEventName);
		if (tFoundit == sgpImplementation->mEvents.end())
			return;
	}
	tFoundit->second->start();
}

void FMod::StopEvent(const string &strEventName, bool bImmediate) {
	auto tFoundIt = sgpImplementation->mEvents.find(strEventName);
	if (tFoundIt == sgpImplementation->mEvents.end())
		return;
	FMOD_STUDIO_STOP_MODE eMode;
	eMode = bImmediate ? FMOD_STUDIO_STOP_IMMEDIATE : FMOD_STUDIO_STOP_ALLOWFADEOUT;
	FMod::ErrorCheck(tFoundIt->second->stop(eMode));
}

bool FMod::IsEventPlaying(const string &strEventName) const {
	auto tFoundIt = sgpImplementation->mEvents.find(strEventName);
	if (tFoundIt == sgpImplementation->mEvents.end())
		return false;

	FMOD_STUDIO_PLAYBACK_STATE* state = NULL;
	if (tFoundIt->second->getPlaybackState(state) == FMOD_STUDIO_PLAYBACK_PLAYING) {
		return true;
	}
	return false;
}

void FMod::GetEventParameter(const string &strEventName, const string &strParameterName, float* parameter) {
	auto tFoundIt = sgpImplementation->mEvents.find(strEventName);
	if (tFoundIt == sgpImplementation->mEvents.end())
		return;
	FMOD::Studio::ParameterInstance* pParameter = NULL;
	FMod::ErrorCheck(tFoundIt->second->getParameter(strParameterName.c_str(), &pParameter));
	FMod::ErrorCheck(pParameter->getValue(parameter));
}

void FMod::SetEventParameter(const string &strEventName, const string &strParameterName, float fValue) {
	auto tFoundIt = sgpImplementation->mEvents.find(strEventName);
	if (tFoundIt == sgpImplementation->mEvents.end())
		return;
	FMOD::Studio::ParameterInstance* pParameter = NULL;
	FMod::ErrorCheck(tFoundIt->second->getParameter(strParameterName.c_str(), &pParameter));
	FMod::ErrorCheck(pParameter->setValue(fValue));
}

int FMod::ErrorCheck(FMOD_RESULT result) {
	if (result != FMOD_OK){
		cout << "FMOD ERROR " << result << endl;
		return 1;
	}
	// cout << "FMOD all good" << endl;
	return 0;
}

void FMod::Shutdown() {
	delete sgpImplementation;
}

void FMod::_bind_methods() {
	//ClassDB::bind_method(D_METHOD("init"), &FMod::Init);
	//ClassDB::bind_method(D_METHOD("update"), &FMod::Update);
	//ClassDB::bind_method(D_METHOD("load_sound", "sound_name", "3d","looping", "stream"), &FMod::LoadSound);
	//ClassDB::bind_method(D_METHOD("unload_sound", "sound_name"), &FMod::UnLoadSound);
	//ClassDB::bind_method(D_METHOD("play_sounds","sound_name","position","volume"), &FMod::PlaySound);
}