// Use the actual upstream disc validator/TOC parser without a desktop GL game build.
// Metadata only. Does not extract bytes, render, or run any game instructions.
#include <setup/iso_extract.h>
#include <cstdio>
int main(int argc, char** argv) {
    if(argc != 2) { std::fputs("usage: disc_audit <owned ISO>\n", stderr); return 2; }
    const auto check = rac1::setup::validate_iso(argv[1]);
    std::printf("UPSTREAM_ISO_VALID=%s\nSIZE=%llu\nSERIAL=%s\nREGION=%s\n",
        check.valid ? "YES" : "NO", static_cast<unsigned long long>(check.size),
        check.disc ? check.disc->serial : "UNSUPPORTED", check.disc ? check.disc->region : "UNKNOWN");
    if(!check.message.empty()) std::fprintf(stderr,"UPSTREAM_DIAGNOSTIC=%s\n",check.message.c_str());
    return check.valid && check.disc && check.serial == "SCUS_971.99" ? 0 : 1;
}
