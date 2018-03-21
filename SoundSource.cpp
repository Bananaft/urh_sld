//
// Copyright (c) 2008-2016 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Audio/Audio.h"
#include "../Audio/AudioEvents.h"
#include "../Audio/Sound.h"
#include "../Audio/SoundSource.h"
#include "../Audio/SoundStream.h"
#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Node.h"
#include "../Scene/ReplicationState.h"
#include "soloud.h"
#include "soloud_speech.h"
#include "soloud_wav.h"

#include "../DebugNew.h"

namespace Urho3D
{

#define INC_POS_LOOPED() \
    pos += intAdd; \
    fractPos += fractAdd; \
    if (fractPos > 65535) \
    { \
        fractPos &= 65535; \
        ++pos; \
    } \
    while (pos >= end) \
        pos -= (end - repeat); \

#define INC_POS_ONESHOT() \
    pos += intAdd; \
    fractPos += fractAdd; \
    if (fractPos > 65535) \
    { \
        fractPos &= 65535; \
        ++pos; \
    } \
    if (pos >= end) \
    { \
        pos = 0; \
        break; \
    } \

#define INC_POS_STEREO_LOOPED() \
    pos += (intAdd << 1); \
    fractPos += fractAdd; \
    if (fractPos > 65535) \
    { \
        fractPos &= 65535; \
        pos += 2; \
    } \
    while (pos >= end) \
        pos -= (end - repeat); \

#define INC_POS_STEREO_ONESHOT() \
    pos += (intAdd << 1); \
    fractPos += fractAdd; \
    if (fractPos > 65535) \
    { \
        fractPos &= 65535; \
        pos += 2; \
    } \
    if (pos >= end) \
    { \
        pos = 0; \
        break; \
    } \

#define GET_IP_SAMPLE() (((((int)pos[1] - (int)pos[0]) * fractPos) / 65536) + (int)pos[0])

#define GET_IP_SAMPLE_LEFT() (((((int)pos[2] - (int)pos[0]) * fractPos) / 65536) + (int)pos[0])

#define GET_IP_SAMPLE_RIGHT() (((((int)pos[3] - (int)pos[1]) * fractPos) / 65536) + (int)pos[1])

static const float AUTOREMOVE_DELAY = 0.25f;

static const int STREAM_SAFETY_SAMPLES = 4;

extern const char* AUDIO_CATEGORY;


SoundSource::SoundSource(Context* context) :
    Component(context),
    soundType_(SOUND_EFFECT),
    frequency_(0.0f),
    gain_(1.0f),
    attenuation_(1.0f),
    panning_(0.0f),
    autoRemoveTimer_(0.0f),
    autoRemove_(false),
    sendFinishedEvent_(false),
    position_(0),
    fractPosition_(0),
    timePosition_(0.0f),
    unusedStreamSize_(0),
	handle_(0)
{
	URHO3D_LOGINFO("SoundSource");
	audio_ = GetSubsystem<Audio>();

    if (audio_)
        audio_->AddSoundSource(this);

    //UpdateMasterGain();

	
}

SoundSource::~SoundSource()
{
	SoLoud::Soloud* soloud = audio_->GetSoLoud();
	soloud->stop(handle_);

	if (audio_)
        audio_->RemoveSoundSource(this);
}

void SoundSource::RegisterObject(Context* context)
{
    context->RegisterFactory<SoundSource>(AUDIO_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Sound", GetSoundAttr, SetSoundAttr, ResourceRef, ResourceRef(Sound::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Type", GetSoundType, SetSoundType, String, SOUND_EFFECT, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Frequency", float, frequency_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Gain", float, gain_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Attenuation", float, attenuation_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Panning", float, panning_, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Playing", IsPlaying, SetPlayingAttr, bool, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Autoremove on Stop", bool, autoRemove_, false, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Play Position", GetPositionAttr, SetPositionAttr, int, 0, AM_FILE);
}

void SoundSource::Play(Sound* sound)
{
   /* if (!audio_)
        return;

    // If no frequency set yet, set from the sound's default
    if (frequency_ == 0.0f && sound)
        SetFrequency(sound->GetFrequency());

    // If sound source is currently playing, have to lock the audio mutex
    if (position_)
    {
        MutexLock lock(audio_->GetMutex());
        PlayLockless(sound);
    }
    else
        PlayLockless(sound);

    // Forget the Sound & Is Playing attribute previous values so that they will be sent again, triggering
    // the sound correctly on network clients even after the initial playback
    if (networkState_ && networkState_->attributes_ && networkState_->previousValues_.Size())
    {
        for (unsigned i = 1; i < networkState_->previousValues_.Size(); ++i)
        {
            // The indexing is different for SoundSource & SoundSource3D, as SoundSource3D removes two attributes,
            // so go by attribute types
            VariantType type = networkState_->attributes_->At(i).type_;
            if (type == VAR_RESOURCEREF || type == VAR_BOOL)
                networkState_->previousValues_[i] = Variant::EMPTY;
        }
    }

  	*/
	//URHO3D_LOGINFO("AAA_SoundSource_Play");
	MarkNetworkUpdate();
	if (sound!=NULL)
	{
		SoLoud::Soloud* soloud = audio_->GetSoLoud();
	
		// Play the sound source (we could do this several times if we wanted)
		SoLoud::Wav *wav = sound->GetWav();
		if (wav != NULL)
		{
			handle_ = soloud->play(*wav);
			soloud->setLooping(handle_, sound->IsLooped());
			//soloud->setVolume(handle_, 0.0f);
			URHO3D_LOGINFO("SoLoud Play: " + String(handle_));
		}
	} else {
		URHO3D_LOGERROR("Unable to play sound, it is NULL");
	}
}

void SoundSource::Play(Sound* sound, float frequency)
{
    SetFrequency(frequency);
    Play(sound);
}

void SoundSource::Play(Sound* sound, float frequency, float gain)
{
    SetFrequency(frequency);
    SetGain(gain);
    Play(sound);
}

void SoundSource::Play(Sound* sound, float frequency, float gain, float panning)
{
    SetFrequency(frequency);
    SetGain(gain);
    SetPanning(panning);
    Play(sound);
}


void SoundSource::Stop()
{
	SoLoud::Soloud* soloud = audio_->GetSoLoud();
	soloud->stop(handle_);
}

void SoundSource::SetSoundType(const String& type)
{
    if (type == SOUND_MASTER)
        return;

    soundType_ = type;
    soundTypeHash_ = StringHash(type);
    UpdateMasterGain();

    MarkNetworkUpdate();
}

void SoundSource::SetFrequency(float frequency)
{
    frequency_ = Clamp(frequency, 0.0f, 535232.0f);
    MarkNetworkUpdate();
}

void SoundSource::SetGain(float gain)
{
    gain_ = Max(gain, 0.0f);
    MarkNetworkUpdate();
}

void SoundSource::SetAttenuation(float attenuation)
{
    attenuation_ = Clamp(attenuation, 0.0f, 1.0f);
    MarkNetworkUpdate();
}

void SoundSource::SetPanning(float panning)
{
    panning_ = Clamp(panning, -1.0f, 1.0f);
    MarkNetworkUpdate();
}

void SoundSource::SetAutoRemove(bool enable)
{
    if (enable == true)
        URHO3D_LOGWARNING("SoundSource::SetAutoRemove is deprecated. Consider using the SoundFinished event instead");

    autoRemove_ = enable;
}

bool SoundSource::IsPlaying() const
{
	SoLoud::Soloud* soloud = audio_->GetSoLoud();
	return soloud->isValidVoiceHandle(handle_); //MUTEXES AND SHIT
}

void SoundSource::SetPlayPosition(signed char* pos)
{

}

void SoundSource::Update(float timeStep)
{
	
	//if (handle_ == NULL) URHO3D_LOGINFO("BAM!1111");
	SoLoud::Soloud* soloud = audio_->GetSoLoud();
	//if (soloud->isValidVoiceHandle(handle_))
	soloud->setVolume(handle_, gain_);
}

void SoundSource::Mix(int* dest, unsigned samples, int mixRate, bool stereo, bool interpolation)
{

}

void SoundSource::UpdateMasterGain()
{
    if (audio_)
        masterGain_ = audio_->GetSoundSourceMasterGain(soundType_);
}

void SoundSource::SetSoundAttr(const ResourceRef& value)
{
	URHO3D_LOGDEBUG("SetSoundAttr");
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	Sound* newSound = cache->GetResource<Sound>(value.name_);
	sound_ = newSound;

	if (IsPlaying())
	{
		Stop();
		Play(newSound);
	}
	URHO3D_LOGDEBUG("Done_SetSoundAttr");
}

void SoundSource::SetPlayingAttr(bool value)
{
	URHO3D_LOGDEBUG("SetPlayingAttr");
	if (value)
    {
        if (!IsPlaying())
            Play(sound_);
    }
    else
        Stop();
	URHO3D_LOGDEBUG("done_SetPlayingAttr");
}

void SoundSource::SetPositionAttr(int value)
{
  //  if (sound_)
   //     SetPlayPosition(sound_->GetStart() + value);
}

ResourceRef SoundSource::GetSoundAttr() const
{
    return GetResourceRef(sound_, Sound::GetTypeStatic());
}

int SoundSource::GetPositionAttr() const
{
 /*   if (sound_ && position_)
        return (int)(GetPlayPosition() - sound_->GetStart());
    else
	*/
        return 0;

}

}
