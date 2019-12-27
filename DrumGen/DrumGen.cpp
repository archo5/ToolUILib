
#define _USE_MATH_DEFINES
#include "../GUI.h"

#include <Windows.h>
#include <mmsystem.h>

float randf()
{
	return rand() / (float)RAND_MAX;
}

struct DrumGenerator : ui::Node
{
	DrumGenerator()
	{
		//Regenerate();
	}
	void Render(UIContainer* ctx) override
	{
		ctx->MakeWithText<ui::Button>("Play")->onClick = [this]() { Regenerate(); Play(); };

		for (int i = 0; i < 3; i++)
		{
			ctx->Make<ui::RadioButtonT<int>>()->Init(state, i)->onChange = [this]() { Rerender(); };
		}
	}
	void Play()
	{
		PCMWAVEFORMAT fmt;
		fmt.wf.wFormatTag = 3;
		fmt.wf.nChannels = 1;
		fmt.wf.nSamplesPerSec = 44100;
		fmt.wf.nAvgBytesPerSec = fmt.wf.nSamplesPerSec * fmt.wf.nChannels * 4;
		fmt.wf.nBlockAlign = 4;
		fmt.wBitsPerSample = 32;
		char* data = new char[46 + genData.size() * 4];
		char* p = data;

		// riff chunk
		memcpy(p, "RIFF", 4);
		p += 4;
		*(uint32_t*)p = 42 + genData.size() * 4;
		p += 4;
		memcpy(p, "WAVE", 4);
		p += 4;

		// format chunk
		memcpy(p, "fmt ", 4);
		p += 4;
		*(uint32_t*)p = sizeof(PCMWAVEFORMAT) + 2;
		p += 4;
		memcpy(p, &fmt, sizeof(PCMWAVEFORMAT));
		p += sizeof(PCMWAVEFORMAT);

		*(uint16_t*)p = 0; // format extension field
		p += 2;

		// data chunk
		memcpy(p, "data", 4);
		p += 4;
		*(uint32_t*)p = genData.size() * 4;
		p += 4;
		memcpy(p, genData.data(), genData.size() * 4);
		p += genData.size() * 4;

		FILE* fp = fopen("gen.wav", "wb");
		fwrite(data, 1, p - data, fp);
		fclose(fp);

		PlaySoundW((PWSTR) data, NULL, SND_MEMORY);
	}
	void Regenerate()
	{
		srand(GetTickCount());
#define NUM_IMPULSES 1500
		float frequencies[NUM_IMPULSES];
		float invhalflifes[NUM_IMPULSES];
		float offsets[NUM_IMPULSES];
		float volumes[NUM_IMPULSES];
		for (int i = 0; i < NUM_IMPULSES; i++)
		{
			frequencies[i] = lerp(30, 15000, randf());
			invhalflifes[i] = lerp(20, 40, randf());
			offsets[i] = lerp(0, 0.01f, randf());
			volumes[i] = lerp(0.5f, 0.8f, randf()) / NUM_IMPULSES;
		}

		genData.resize(22050);
		float invSR = 1.0f / 44100.0f;
		for (int s = 0; s < 22050; s++)
		{
			float o = 0;
			for (int i = 0; i < NUM_IMPULSES; i++)
				o += cosf((s * invSR * frequencies[i] - offsets[i]) * 2 * float(M_PI)) * volumes[i] / exp2f((s * invSR - offsets[i]) * invhalflifes[i]);
			genData[s] = o;
		}
	}
	int state = 0;
	std::vector<float> genData;
};

struct MainWindow : ui::NativeMainWindow
{
	MainWindow()
	{
		SetTitle("Drum generator");
	}
	void OnRender(UIContainer* ctx) override
	{
		ctx->Make<DrumGenerator>();
	}
};

int uimain(int argc, char* argv[])
{
	ui::Application app(argc, argv);
	MainWindow mw;
	mw.SetVisible(true);
	return app.Run();
}
