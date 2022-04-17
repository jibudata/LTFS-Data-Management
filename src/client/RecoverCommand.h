#pragma once

class RecoverCommand: public LTFSDMCommand

{
public:
    RecoverCommand() :
            LTFSDMCommand("recover", ":+hf:")
    {
    }
    ~RecoverCommand()
    {
    }
    void printUsage();
    void doCommand(int argc, char **argv);
};
