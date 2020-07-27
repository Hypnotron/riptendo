#include "file.hpp"
#include "wav.hpp"
#include "ast.hpp"

int main(int argc, char* argv[]) {
    WavFile output{"output.raw"};
    AstFile input{argv[1]};

    input.toWav(output);

    output.flush();
    return 0;
}
