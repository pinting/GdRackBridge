#include "GdRackClient.hpp"

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <iostream>
#include <string.h>
#include <sstream>
#include <locale>

#define RETRY_COUNT 3

HANDLE pipeHandle;

void destroyPipe()
{
    if (pipeHandle != NULL)
    {
        CloseHandle(pipeHandle);
        pipeHandle = NULL;
    }
}

int createPipe(LPTSTR pipeName)
{
    if (pipeHandle != NULL)
    {
        godot::Godot::print("Pipe reused");
        return 0;
    }

    BOOL success = FALSE;
    DWORD mode;

    // Try to open a named pipe; wait for it, if necessary.
    while (1)
    {
        pipeHandle = CreateFile(
            pipeName,  // pipe name
            GENERIC_READ | // read and write access
                GENERIC_WRITE,
            0,             // no sharing
            NULL,          // default security attributes
            OPEN_EXISTING, // opens existing pipe
            0,             // default attributes
            NULL);         // no template file

        // Break if the pipe handle is valid.
        if (pipeHandle != INVALID_HANDLE_VALUE)
        {
            break;
        }

        // Exit if an error other than ERROR_PIPE_BUSY occurs.
        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            godot::Godot::print("Could not open pipe");
            godot::Godot::print(std::to_wstring(GetLastError()).c_str());
            return -1;
        }

        // All pipe instances are busy, so wait for 20 seconds.
        if (!WaitNamedPipe(pipeName, 20000))
        {
            godot::Godot::print("Could not open pipe: 20 second wait timed out");
            return -1;
        }

        // The pipe connected; change to message-read mode.
        mode = PIPE_READMODE_MESSAGE;
        success = SetNamedPipeHandleState(
            pipeHandle,   // pipe handle
            &mode, // new pipe mode
            NULL,    // don't set maximum bytes
            NULL);   // don't set maximum time

        if (!success)
        {
            godot::Godot::print("SetNamedPipeHandleState failed");
            godot::Godot::print(std::to_wstring(GetLastError()).c_str());
            return -1;
        }
    }

    godot::Godot::print("Pipe created");

    return 0;
}

DWORD WINAPI InstanceThread(LPVOID parameter)
{
    const char *message = (const char *)parameter;
    BOOL success = FALSE;
    DWORD writeLength;
    DWORD writeCount;
    LPTSTR pipeName = TEXT("\\\\.\\pipe\\gdrackpipe");

    for (int n = 0; n < RETRY_COUNT; n++)
    {
        if (createPipe(pipeName) != 0)
        {
            destroyPipe();
            continue;
        }

        writeLength = ((DWORD)strlen(message) + 1) * sizeof(CHAR);
        
        godot::Godot::print("Sending message (writeLength, messageBuffer)");
        godot::Godot::print(std::to_string(writeLength).c_str());
        godot::Godot::print(message);

        success = WriteFile(
            pipeHandle,
            message,
            writeLength,
            &writeCount,
            NULL);

        if (!success)
        {
            godot::Godot::print("WriteFile to pipe failed");
            godot::Godot::print(std::to_wstring(GetLastError()).c_str());
            destroyPipe();
            continue;
        }

        break;
    }

    return 0;
}

void GdRackClient::sendData(godot::String data)
{
    // Create a thread for this client.
    const char *threadData = data.alloc_c_string();
    HANDLE threadHandle = NULL;
    DWORD threadId = 0;

    threadHandle = CreateThread(
        NULL,
        0,
        InstanceThread,
        (LPVOID)threadData,
        0,
        &threadId);
    
    if (threadHandle == NULL)
    {
        godot::Godot::print("CreateThread failed");
        godot::Godot::print(std::to_wstring(GetLastError()).c_str());
    }
    else
    {
        CloseHandle(threadHandle);
    }
}

void GdRackClient::_register_methods()
{
    godot::register_method("send_data", &GdRackClient::sendData);
}

void GdRackClient::_init()
{
}