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
#include "../Audio/Sound.h"
#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"

#include "../DebugNew.h"

namespace Urho3D
{

Sound::Sound(Context* context) :
    Resource(context)
{
	URHO3D_LOGINFO("Sound");
	audio_ = GetSubsystem<Audio>();
}

Sound::~Sound()
{
	//SoLoud::Soloud* soloud = audio_->GetSoLoud();
	//soloud->stopAudioSource(wav_);
}

void Sound::RegisterObject(Context* context)
{
    context->RegisterFactory<Sound>();
}

bool Sound::BeginLoad(Deserializer& source)
{
    URHO3D_PROFILE(LoadSound);
	
    bool success;
   
	int rslt;
	FileSystem* fc = context_->GetSubsystem<FileSystem>(); //fc->GetCurrentDir()
	String path = fc->GetProgramDir() + "Data/" + source.GetName();
	
	rslt = wav_.load(path.CString());
	URHO3D_LOGINFO("SoLoud load ogg: " + path + " Result: " + String(rslt));
	//wav_.setSingleInstance(true);
	//wav_.setVolume(0.);
	//wav_.set3dProcessing(true);
	
	return true;

}

void Sound::LoadParameters()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	String xmlName = ReplaceExtension(GetName(), ".xml");

	SharedPtr<XMLFile> file(cache->GetTempResource<XMLFile>(xmlName, false));
	if (!file)
		return;

	XMLElement rootElem = file->GetRoot();
	XMLElement paramElem = rootElem.GetChild();

	for (XMLElement paramElem = rootElem.GetChild(); paramElem; paramElem = paramElem.GetNext())
	{
		String name = paramElem.GetName();

		if (name == "loop")
		{
			if (paramElem.HasAttribute("enable"))
				looped_ = paramElem.GetBool("enable");
			//if (paramElem.HasAttribute("start") && paramElem.HasAttribute("end"))
			//	SetLoop((unsigned)paramElem.GetInt("start"), (unsigned)paramElem.GetInt("end"));
		}
	}
}

SoLoud::Wav* Sound::GetWav()
{
	
	return &wav_;
}

}
