#include "murli_context.hpp"
#include "../display/views/splash_view.cpp"
#include "states/no_mod_state.cpp"
#include "states/receive_length_state.cpp"

namespace Murli
{
    MurliContext::MurliContext()
    {
        _noModState = std::make_shared<NoModState>();
        currentState = _noModState;
    }

    void MurliContext::setup()
    {
        _display.init();
        _display.setView(std::make_shared<Murli::SplashView>());
        _display.setLeftStatus("Starting Mesh...");
        _display.loop();
        
        _socketServer.onCommandReceived([this](MurliCommand command) { onSocketServerCommandReceived(command); });
        _socketServer.onMeshConnection([this]() { onSocketServerMeshConnection(); });

        _wifi.startMesh();
    }

    void MurliContext::loop()
    {
        char heapString[10];
        itoa(ESP.getFreeHeap(), heapString, 10);
        _display.setLeftStatus(heapString);

        checkModuleInserted();
        checkWriteRequest();
        
        currentState->run(*this);
        _socketServer.loop();
        _led.loop();
        _display.loop();
    }

    bool MurliContext::isModInserted() const
    {
        return digitalRead(D6) == HIGH;
    }

    void MurliContext::checkModuleInserted()
    {
        if(!isModInserted()) currentState = _noModState;
    }

    void MurliContext::checkWriteRequest()
    {
        if(Serial.available() > 0
            && !writeRequested
            && isModInserted())
        {
            int incomingByte = Serial.read();
            if(incomingByte == 30)
            {
                Serial.write(30);
                writeRequested = true;
                currentState = std::make_shared<ReceiveLengthState>();
            }
        }
    }

    void MurliContext::startMeshCount()
    {
        Serial.println("Connection state changed. Starting MESH_COUNT_REQUEST ...");
        if(_socketServer.connectedClients() == 0)
        {
            Serial.println("No clients connected!");
            Serial.printf("muRLi alone with %d LEDs :(\n", LED_COUNT);
            _meshLedCount = LED_COUNT;
        }
        else
        {
            MurliCommand command = { millis(), Murli::MESH_COUNT_REQUEST, 0, 0, LED_COUNT, 0, 1 };
            _socketServer.broadcast(command);
        }
    }

    void MurliContext::onSocketServerMeshConnection()
    {
        startMeshCount();
    }

    void MurliContext::onSocketServerCommandReceived(MurliCommand command)
    {
        MurliCommand updateCommand;
        switch (command.command)
        {
            case Murli::MESH_CONNECTION:
                startMeshCount();
                break;
            case Murli::MESH_COUNTED:
                _meshLedCount = command.meshLedCount;
                Serial.printf("MESH_COUNT done with %d LEDs on the longest route.\n", _meshLedCount);

                Serial.println("Distributing result of count ...");
                updateCommand = { millis(), MESH_UPDATE, 0, 0, _meshLedCount, LED_COUNT, 1 };
                _socketServer.broadcast(updateCommand);
                break;
            case Murli::MESH_UPDATED:
                Serial.println("Mesh updated - Resetting state ...");
                currentState = std::make_shared<NoModState>();
                break;
            default:
                break;
        }
    }

    LED& MurliContext::getLed() { return _led; }
    Rom24LC32A& MurliContext::getRom() { return _rom; }
    Display& MurliContext::getDisplay() { return _display; }
    uint32_t MurliContext::getMeshLedCount() { return _meshLedCount; }
    SocketServer& MurliContext::getSocketServer() { return _socketServer; }
}