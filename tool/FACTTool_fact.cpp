/* FACT - XACT Reimplementation for FNA
 * Copyright 2009-2017 Ethan Lee and the MonoGame Team
 *
 * Released under the Microsoft Public License.
 * See LICENSE for details.
 */

/* This is where the actual tool lives! Try to do your work in here! */

#include <FACT_internal.h> /* DO NOT INCLUDE THIS IN REAL CODE! */

#include "imgui.h"

#include <SDL.h>
#include <vector>
#include <string>

bool openEngineShow = true;

std::vector<FACTAudioEngine*> engines;
std::vector<std::string> engineNames;
std::vector<bool> engineShows;

std::vector<FACTSoundBank*> soundBanks;
std::vector<bool> soundbankShows;

std::vector<FACTWaveBank*> waveBanks;
std::vector<bool> wavebankShows;

bool show_test_window = false;
void FACTTool_Update()
{
	ImGui::ShowTestWindow(&show_test_window);

	uint8_t *buf;
	uint32_t len;
	SDL_RWops *fileIn;
	#define OPENFILE(name) \
		fileIn = SDL_RWFromFile(name, "rb"); \
		len = SDL_RWsize(fileIn); \
		buf = (uint8_t*) SDL_malloc(len); \
		SDL_RWread(fileIn, buf, len, 1); \
		SDL_RWclose(fileIn);

	/* Menu bar */
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Window"))
		{
			if (ImGui::MenuItem("Open AudioEngine", NULL, openEngineShow, true))
			{
				openEngineShow = !openEngineShow;
			}
			for (size_t i = 0; i < engines.size(); i += 1)
			{
				if (ImGui::MenuItem(
					engineNames[i].c_str(),
					NULL,
					engineShows[i],
					true
				)) {
					engineShows[i] = !engineShows[i];
				}
			}
			for (size_t i = 0; i < soundBanks.size(); i += 1)
			{
				if (ImGui::MenuItem(
					soundBanks[i]->name,
					NULL,
					soundbankShows[i],
					true
				)) {
					soundbankShows[i] = !soundbankShows[i];
				}
			}
			for (size_t i = 0; i < waveBanks.size(); i += 1)
			{
				if (ImGui::MenuItem(
					waveBanks[i]->name,
					NULL,
					wavebankShows[i],
					true
				)) {
					wavebankShows[i] = !wavebankShows[i];
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	/* Open AudioEngine */
	if (openEngineShow && ImGui::Begin("Open AudioEngine"))
	{
		static char enginename[64] = "AudioEngine.xgs";
		ImGui::InputText("", enginename, 64);
		ImGui::SameLine();
		if (ImGui::Button("Open"))
		{
			/* Load up file... */
			OPENFILE(enginename)
			FACTRuntimeParameters params;
			params.pGlobalSettingsBuffer = buf;
			params.globalSettingsBufferSize = len;

			/* Create engine... */
			FACTAudioEngine *engine;
			FACTCreateEngine(0, &engine);
			FACTAudioEngine_Initialize(engine, &params);
			SDL_free(buf);

			/* Add to UI... */
			engines.push_back(engine);
			engineNames.push_back(enginename);
			engineShows.push_back(true);
		}
		ImGui::End();
	}

	/* AudioEngine windows */
	for (size_t i = 0; i < engines.size(); i += 1)
	if (engineShows[i] && ImGui::Begin(engineNames[i].c_str()))
	{
		/* Categories */
		if (ImGui::CollapsingHeader("Categories"))
		for (uint16_t j = 0; j < engines[i]->categoryCount; j += 1)
		if (ImGui::TreeNode(engines[i]->categoryNames[j]))
		{
			ImGui::Text(
				"Max Instances: %d",
				engines[i]->categories[j].maxInstances
			);
			ImGui::Text(
				"Fade-in (ms): %d",
				engines[i]->categories[j].fadeInMS
			);
			ImGui::Text(
				"Fade-out (ms): %d",
				engines[i]->categories[j].fadeOutMS
			);
			ImGui::Text(
				"Instance Behavior: %X",
				engines[i]->categories[j].instanceBehavior
			);
			ImGui::Text(
				"Parent Category Index: %d",
				engines[i]->categories[j].parentCategory
			);
			ImGui::Text(
				"Base Volume: %d",
				engines[i]->categories[j].volume
			);
			ImGui::Text(
				"Visibility: %X",
				engines[i]->categories[j].visibility
			);
			ImGui::TreePop();
		}

		/* Variables */
		if (ImGui::CollapsingHeader("Variables"))
		for (uint16_t j = 0; j < engines[i]->variableCount; j += 1)
		if (ImGui::TreeNode(engines[i]->variableNames[j]))
		{
			ImGui::Text(
				"Accessibility: %X",
				engines[i]->variables[j].accessibility
			);
			ImGui::Text(
				"Initial Value: %f",
				engines[i]->variables[j].initialValue
			);
			ImGui::Text(
				"Min Value: %f",
				engines[i]->variables[j].minValue
			);
			ImGui::Text(
				"Max Value: %f",
				engines[i]->variables[j].maxValue
			);
			ImGui::TreePop();
		}

		/* RPCs */
		if (ImGui::CollapsingHeader("RPCs"))
		for (uint16_t j = 0; j < engines[i]->rpcCount; j += 1)
		if (ImGui::TreeNode(
			(void*) (intptr_t) j,
			"Code %d",
			engines[i]->rpcCodes[j]
		)) {
			ImGui::Text(
				"Variable Index: %d",
				engines[i]->rpcs[j].variable
			);
			ImGui::Text(
				"Parameter: %d",
				engines[i]->rpcs[j].parameter
			);
			ImGui::Text(
				"Point Count: %d",
				engines[i]->rpcs[j].pointCount
			);
			if (ImGui::TreeNode("Points"))
			{
				for (uint8_t k = 0; k < engines[i]->rpcs[j].pointCount; k += 1)
				if (ImGui::TreeNode(
					(void*) (intptr_t) k,
					"Point #%d",
					k
				)) {
					ImGui::Text(
						"Coordinate: (%f, %f)",
						engines[i]->rpcs[j].points[k].x,
						engines[i]->rpcs[j].points[k].y
					);
					ImGui::Text(
						"Type: %d\n",
						engines[i]->rpcs[j].points[k].type
					);
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

		/* DSP Presets */
		if (ImGui::CollapsingHeader("DSP Presets"))
		for (uint16_t j = 0; j < engines[i]->dspPresetCount; j += 1)
		if (ImGui::TreeNode(
			(void*) (intptr_t) j,
			"Code %d",
			engines[i]->dspPresetCodes[j]
		)) {
			ImGui::Text(
				"Accessibility: %X",
				engines[i]->dspPresets[j].accessibility
			);
			ImGui::Text(
				"Parameter Count: %d",
				engines[i]->dspPresets[j].parameterCount
			);
			if (ImGui::TreeNode("Parameters"))
			{
				for (uint32_t k = 0; k < engines[i]->dspPresets[j].parameterCount; k += 1)
				if (ImGui::TreeNode(
					(void*) (intptr_t) k,
					"Parameter #%d",
					k
				)) {
					ImGui::Text(
						"Initial Value: %f",
						engines[i]->dspPresets[j].parameters[k].value
					);
					ImGui::Text(
						"Min Value: %f",
						engines[i]->dspPresets[j].parameters[k].minVal
					);
					ImGui::Text(
						"Max Value: %f",
						engines[i]->dspPresets[j].parameters[k].maxVal
					);
					ImGui::Text(
						"Unknown u16: %d",
						engines[i]->dspPresets[j].parameters[k].unknown
					);
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

		ImGui::Separator();

		/* Open SoundBank */
		static char soundbankname[64] = "Sound Bank.xsb";
		ImGui::InputText("", soundbankname, 64);
		ImGui::SameLine();
		if (ImGui::Button("Open SoundBank"))
		{
			/* Load up file... */
			OPENFILE(soundbankname)

			/* Create SoundBank... */
			FACTSoundBank *sb;
			FACTAudioEngine_CreateSoundBank(
				engines[i],
				buf,
				len,
				0,
				0,
				&sb
			);
			SDL_free(buf);

			/* Add to UI... */
			soundBanks.push_back(sb);
			soundbankShows.push_back(true);
		}

		/* Open WaveBank */
		static char wavebankname[64] = "Wave Bank.xwb";
		ImGui::InputText("", wavebankname, 64);
		ImGui::SameLine();
		if (ImGui::Button("Open WaveBank"))
		{
			/* Load up file... */
			OPENFILE(wavebankname)

			/* Create WaveBank... */
			FACTWaveBank *wb;
			FACTAudioEngine_CreateInMemoryWaveBank(
				engines[i],
				buf,
				len,
				0,
				0,
				&wb
			);

			/* Add to UI... */
			waveBanks.push_back(wb);
			wavebankShows.push_back(true);
		}

		ImGui::Separator();

		/* Close file */
		if (ImGui::Button("Close AudioEngine"))
		{
			/* Destroy SoundBank windows... */
			FACTSoundBank *sb = engines[i]->sbList;
			while (sb != NULL)
			{
				for (size_t j = 0; j < soundBanks.size(); j += 1)
				{
					if (sb == soundBanks[j])
					{
						sb = sb->next;
						soundBanks.erase(soundBanks.begin() + j);
						soundbankShows.erase(soundbankShows.begin() + j);
						break;
					}
				}
			}

			/* Destroy WaveBank windows... */
			FACTWaveBank *wb = engines[i]->wbList;
			while (wb != NULL)
			{
				for (size_t j = 0; j < waveBanks.size(); j += 1)
				{
					if (wb == waveBanks[j])
					{
						wb = wb->next;
						waveBanks.erase(waveBanks.begin() + j);
						wavebankShows.erase(wavebankShows.begin() + j);
						break;
					}
				}
			}

			/* Close file, finally */
			FACTAudioEngine_Shutdown(engines[i]);
			engines.erase(engines.begin() + i);
			engineNames.erase(engineNames.begin() + i);
			engineShows.erase(engineShows.begin() + i);
			i -= 1;
		}

		/* We out. */
		ImGui::End();
	}

	/* SoundBank windows */
	for (size_t i = 0; i < soundBanks.size(); i += 1)
	if (soundbankShows[i] && ImGui::Begin(soundBanks[i]->name))
	{
		/* Close file */
		if (ImGui::Button("Close SoundBank"))
		{
			FACTSoundBank_Destroy(soundBanks[i]);
			soundBanks.erase(soundBanks.begin() + i);
			soundbankShows.erase(soundbankShows.begin() + i);
			i -= 1;
			continue;
		}

		ImGui::Separator();

		/* WaveBank Dependencies */
		if (ImGui::CollapsingHeader("WaveBank Dependencies"))
		for (uint8_t j = 0; j < soundBanks[i]->wavebankCount; j += 1)
		{
			ImGui::Text(soundBanks[i]->wavebankNames[j]);
		}

		/* Cues */
		if (ImGui::CollapsingHeader("Cues"))
		for (uint16_t j = 0; j < soundBanks[i]->cueCount; j += 1)
		if (ImGui::TreeNode(soundBanks[i]->cueNames[j]))
		{
			ImGui::Text(
				"Flags: %X",
				soundBanks[i]->cues[j].flags
			);
			ImGui::Text(
				"Sound/Variation Code: %d",
				soundBanks[i]->cues[j].sbCode
			);
			ImGui::Text(
				"Transition Offset: %d",
				soundBanks[i]->cues[j].transitionOffset
			);
			ImGui::Text(
				"Instance Limit: %d",
				soundBanks[i]->cues[j].instanceLimit
			);
			ImGui::Text(
				"Fade-out (ms): %d",
				soundBanks[i]->cues[j].fadeIn
			);
			ImGui::Text(
				"Fade-out (ms): %d",
				soundBanks[i]->cues[j].fadeOut
			);
			ImGui::Text(
				"Max Instance Behavior: %d",
				soundBanks[i]->cues[j].maxInstanceBehavior
			);
			ImGui::TreePop();
		}

		/* Sounds */
		if (ImGui::CollapsingHeader("Sounds"))
		for (uint16_t j = 0; j < soundBanks[i]->soundCount; j += 1)
		{
		}

		/* Variations */
		if (ImGui::CollapsingHeader("Variations"))
		for (uint16_t j = 0; j < soundBanks[i]->variationCount; j += 1)
		if (ImGui::TreeNode(
			(void*) (intptr_t) j,
			"Code %d",
			soundBanks[i]->variationCodes[j]
		)) {
			ImGui::Text(
				"Flags: %X",
				soundBanks[i]->variations[j].flags
			);
			ImGui::Text(
				"Interactive Variable Index: %d",
				soundBanks[i]->variations[j].variable
			);
			ImGui::Text(
				"Entry Count: %X",
				soundBanks[i]->variations[j].entryCount
			);
			if (ImGui::TreeNode("Entries"))
			{
				for (uint16_t k = 0; k < soundBanks[i]->variations[j].entryCount; k += 1)
				if (ImGui::TreeNode(
					(void*) (intptr_t) k,
					"Entry #%d",
					k
				)) {
					if (soundBanks[i]->variations[j].entries[k].isComplex)
					{
						ImGui::Text("Complex Variation");
						ImGui::Text(
							"Sound Code: %d",
							soundBanks[i]->variations[j].entries[k].soundCode
						);
					}
					else
					{
						ImGui::Text("Simple Variation");
						ImGui::Text(
							"Track Index: %d",
							soundBanks[i]->variations[j].entries[k].track
						);
						ImGui::Text(
							"WaveBank Index: %d",
							soundBanks[i]->variations[j].entries[k].wavebank
						);
					}
					ImGui::Text(
						"Min Weight: %f",
						soundBanks[i]->variations[j].entries[k].minWeight
					);
					ImGui::Text(
						"Max Weight: %f",
						soundBanks[i]->variations[j].entries[k].maxWeight
					);
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

		/* We out. */
		ImGui::End();
	}

	/* WaveBank windows */
	for (size_t i = 0; i < waveBanks.size(); i += 1)
	if (wavebankShows[i] && ImGui::Begin(waveBanks[i]->name))
	{
		/* Close file */
		if (ImGui::Button("Close WaveBank"))
		{
			FACTWaveBank_Destroy(waveBanks[i]);
			waveBanks.erase(waveBanks.begin() + i);
			wavebankShows.erase(wavebankShows.begin() + i);
			i -= 1;
			continue;
		}

		/* Giant table of wavedata entries */
		ImGui::Columns(12, "wavebankentries");
		ImGui::Separator();
		ImGui::Text("Entry");		ImGui::NextColumn();
		ImGui::Text("Flags");		ImGui::NextColumn();
		ImGui::Text("Duration");	ImGui::NextColumn();
		ImGui::Text("Format Tag");	ImGui::NextColumn();
		ImGui::Text("Channels");	ImGui::NextColumn();
		ImGui::Text("Sample Rate");	ImGui::NextColumn();
		ImGui::Text("Block Align");	ImGui::NextColumn();
		ImGui::Text("Bit Depth");	ImGui::NextColumn();
		ImGui::Text("Play Offset");	ImGui::NextColumn();
		ImGui::Text("Play Length");	ImGui::NextColumn();
		ImGui::Text("Loop Offset");	ImGui::NextColumn();
		ImGui::Text("Loop Length");	ImGui::NextColumn();
		ImGui::Separator();
		for (uint32_t j = 0; j < waveBanks[i]->entryCount; j += 1)
		{
			ImGui::Text("%d", j);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].dwFlags);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].Duration);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].Format.wFormatTag);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].Format.nChannels);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].Format.nSamplesPerSec);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].Format.wBlockAlign);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].Format.wBitsPerSample);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].PlayRegion.dwOffset);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].PlayRegion.dwLength);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].LoopRegion.dwStartSample);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].LoopRegion.dwTotalSamples);
			ImGui::NextColumn();
		}
		ImGui::Columns(1);
		ImGui::Separator();

		/* We out. */
		ImGui::End();
	}
}
