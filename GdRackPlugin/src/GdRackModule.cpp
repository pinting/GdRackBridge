#include "plugin.hpp"

#include <windows.h> 
#include <stdio.h> 
#include <tchar.h>
#include <strsafe.h>
#include <iostream>
#include <string.h>
#include <sstream>

#define BUFSIZE 512

struct GdRackModule;
struct ThreadData;

DWORD WINAPI InstanceThread(LPVOID parameter);
DWORD WINAPI ServerThread(LPVOID parameter);
void createServer(GdRackModule *context);

struct ThreadData
{
	HANDLE pipeHandle;
	GdRackModule *context;

	ThreadData(GdRackModule *context, HANDLE pipeHandle)
	{
		this->context = context;
		this->pipeHandle = pipeHandle;
	}
};


struct GdRackModule : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		SINE_OUTPUT_00,
		SINE_OUTPUT_01,
		SINE_OUTPUT_02,
		SINE_OUTPUT_03,
		SINE_OUTPUT_04,
		SINE_OUTPUT_05,
		SINE_OUTPUT_06,
		SINE_OUTPUT_07,
		OUTPUTS_LEN
	};
	enum LightId {
		BLINK_LIGHT_00,
		BLINK_LIGHT_01,
		BLINK_LIGHT_02,
		BLINK_LIGHT_03,
		BLINK_LIGHT_04,
		BLINK_LIGHT_05,
		BLINK_LIGHT_06,
		BLINK_LIGHT_07,
		LIGHTS_LEN
	};

	double state[LIGHTS_LEN] = { 0 };

	GdRackModule() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configOutput(SINE_OUTPUT_00, "");
		configOutput(SINE_OUTPUT_01, "");
		configOutput(SINE_OUTPUT_02, "");
		configOutput(SINE_OUTPUT_03, "");
		configOutput(SINE_OUTPUT_04, "");
		configOutput(SINE_OUTPUT_05, "");
		configOutput(SINE_OUTPUT_06, "");
		configOutput(SINE_OUTPUT_07, "");
	}

	void process(const ProcessArgs& args) override
	{
		for (int index = 0; index < OUTPUTS_LEN; index++)
		{
			int outputIndex = SINE_OUTPUT_00 + index;
			int lightIndex = BLINK_LIGHT_00 + index;

			outputs[outputIndex].setVoltage(state[index]);
			lights[lightIndex].setBrightness(abs(state[index] / 10.0));
		}
	}
	
	void callback(int index, double new_state)
	{
		state[index] = new_state;
	}

	void onAdd() override
	{
		createServer(this);
	}

};

struct GdRackModuleWidget : ModuleWidget {
	GdRackModuleWidget(GdRackModule* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GdRackModuleWidget.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.189, 28.264)), module, GdRackModule::SINE_OUTPUT_00));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.317, 40.382)), module, GdRackModule::SINE_OUTPUT_01));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.852, 52.677)), module, GdRackModule::SINE_OUTPUT_02));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.852, 64.704)), module, GdRackModule::SINE_OUTPUT_03));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.119, 77.265)), module, GdRackModule::SINE_OUTPUT_04));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.852, 89.293)), module, GdRackModule::SINE_OUTPUT_05));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.852, 101.587)), module, GdRackModule::SINE_OUTPUT_06));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.584, 114.95)), module, GdRackModule::SINE_OUTPUT_07));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(8.558, 28.215)), module, GdRackModule::BLINK_LIGHT_00));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(8.686, 40.333)), module, GdRackModule::BLINK_LIGHT_01));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(9.221, 52.628)), module, GdRackModule::BLINK_LIGHT_02));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(9.221, 64.655)), module, GdRackModule::BLINK_LIGHT_03));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(9.488, 77.216)), module, GdRackModule::BLINK_LIGHT_04));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(9.221, 89.243)), module, GdRackModule::BLINK_LIGHT_05));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(9.221, 101.538)), module, GdRackModule::BLINK_LIGHT_06));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(8.954, 114.901)), module, GdRackModule::BLINK_LIGHT_07));
	}
};

void splitRequest(char *buffer, char **left, char **right, DWORD readCount)
{
	*left = buffer;
	*right = buffer;

	for (DWORD offset = 0; offset < readCount - 1; offset++)
	{
		char *current = buffer + offset;
		
		if (*current == ';')
		{
			*current = '\0';
			*right = current + 1;
			break;
		}
	}

	if (*left == *right)
	{
		throw std::runtime_error("Request split failed");
	}
}

void parseRequest(ThreadData *threadData, const TCHAR *requestBuffer, DWORD readCount)
{
	char buffer[BUFSIZE];
	size_t size = BUFSIZE * sizeof(TCHAR);

	StringCchPrintfA(buffer, size, "%hs", requestBuffer);
	
	DEBUG("Request: %s\n", buffer);

	try
	{
		char *left;
		char *right;
		
		splitRequest(buffer, &left, &right, readCount);

		int index = std::stoi(left);
		double state = std::stod(right);
	
		DEBUG("Parsed: %d - %f\n", index, state);
		
		threadData->context->callback(index, state);
	}
	catch(const std::exception& e)
	{
		DEBUG("Request could not be parsed: %s\n", e.what());
	}
}

// This routine is a thread processing function to read from and reply to a client
// via the open pipe connection passed from the main loop. Note this allows
// the main loop to continue executing, potentially creating more threads of
// of this procedure to run concurrently, depending on the number of incoming
// client connections.
DWORD WINAPI InstanceThread(LPVOID parameter)
{
	HANDLE heapHandle = GetProcessHeap();
	TCHAR *requestBuffer = (TCHAR *)HeapAlloc(heapHandle, 0, BUFSIZE * sizeof(TCHAR));

	DWORD readCount = 0;
	BOOL success = FALSE;
	ThreadData *threadData = NULL;

	// Do some extra error checking since the app will keep running even if this
	// thread fails.
	if (parameter == NULL)
	{
		DEBUG("InstanceThread got an unexpected NULL value in parameter.\n");

		if (requestBuffer != NULL)
		{
			HeapFree(heapHandle, 0, requestBuffer);
		}

		return (DWORD)-1;
	}

	// Print verbose messages. In production code, this should be for debugging only.
	DEBUG("InstanceThread created, receiving and processing messages.\n");

	threadData = (ThreadData *)parameter;

	// Loop until done reading
	while (1)
	{
		// Read client requests from the pipe. This simplistic code only allows messages
		// up to BUFSIZE characters in length.
		success = ReadFile(
			threadData->pipeHandle,
			requestBuffer,
			BUFSIZE * sizeof(TCHAR),
			&readCount,
			NULL);

		if (!success || readCount == 0)
		{
			if (GetLastError() == ERROR_BROKEN_PIPE)
			{
				DEBUG("InstanceThread: client disconnected.\n");
			}
			else
			{
				DEBUG("InstanceThread ReadFile failed, GLE=%ld.\n", GetLastError());
			}
			
			break;
		}
		
		parseRequest(threadData, requestBuffer, readCount);
	}

	// Flush the pipe to allow the client to read the pipe's contents
	// before disconnecting. Then disconnect the pipe, and close the
	// handle to this pipe instance.
	FlushFileBuffers(threadData->pipeHandle);
	DisconnectNamedPipe(threadData->pipeHandle);
	CloseHandle(threadData->pipeHandle);

	HeapFree(heapHandle, 0, requestBuffer);
	delete(threadData);

	DEBUG("InstanceThread exiting.\n");
	return 1;
}

DWORD WINAPI ServerThread(LPVOID parameter)
{
	BOOL connected = FALSE;
	DWORD threadId = 0;
	HANDLE pipeHandle = INVALID_HANDLE_VALUE;
	HANDLE threadHandle = NULL;
	LPCTSTR pipeName = TEXT("\\\\.\\pipe\\gdrackpipe");
	GdRackModule *context = (GdRackModule *)parameter;

	// The main loop creates an instance of the named pipe and
	// then waits for a client to connect to it. When the client
	// connects, a thread is created to handle communications
	// with that client, and this loop is free to wait for the
	// next client connect request. It is an infinite loop.
	for (;;)
	{
		DEBUG("\nPipe Server: Main thread awaiting client connection on %ls\n", pipeName);

		pipeHandle = CreateNamedPipe(
			pipeName,				// pipe name
			PIPE_ACCESS_DUPLEX,			// read/write access
			PIPE_TYPE_MESSAGE |			// message type pipe
				PIPE_READMODE_MESSAGE | // message-read mode
				PIPE_WAIT,				// blocking mode
			PIPE_UNLIMITED_INSTANCES,	// max. instances
			BUFSIZE,					// output buffer size
			BUFSIZE,					// input buffer size
			0,							// client time-out
			NULL);						// default security attribute

		if (pipeHandle == INVALID_HANDLE_VALUE)
		{
			DEBUG("CreateNamedPipe failed, GLE=%ld.\n", GetLastError());
			return -1;
		}

		// Wait for the client to connect; if it succeeds,
		// the function returns a nonzero value. If the function
		// returns zero, GetLastError returns ERROR_PIPE_CONNECTED.
		connected = ConnectNamedPipe(pipeHandle, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		if (connected)
		{
			DEBUG("Client connected, creating a processing thread.\n");

			// Create a thread for this client.
			ThreadData *threadData = new ThreadData(context, pipeHandle);
			threadHandle = CreateThread(
				NULL,					// no security attribute
				0,						// default stack size
				InstanceThread,		 	// thread proc
				(LPVOID)threadData,		// thread parameter
				0,						// not suspended
				&threadId);			// returns thread ID

			if (threadHandle == NULL)
			{
				DEBUG("CreateThread failed, GLE=%ld.\n", GetLastError());
				return -1;
			}
			else
			{
				CloseHandle(threadHandle);
			}
		}
		else
		{
			// The client could not connect, so close the pipe.
			CloseHandle(pipeHandle);
		}
	}

	return 0;
}

void createServer(GdRackModule *context)
{
	HANDLE threadHandle;
	DWORD threadId;

	DEBUG("Create server");

	// Create a thread for this client.
	threadHandle = CreateThread(
		NULL,				// no security attribute
		0,					// default stack size
		ServerThread,		// thread proc
		(LPVOID)context,	// thread parameter
		0,					// not suspended
		&threadId);		// returns thread ID

	if (threadHandle == NULL)
	{
		DEBUG("CreateThread failed, GLE=%ld.\n", GetLastError());
	}
	else
	{
		CloseHandle(threadHandle);
	}
}

Model *modelGdRackModule = createModel<GdRackModule, GdRackModuleWidget>("GdRackModule");