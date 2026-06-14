#include "veyra/ipc/PipeIo.h"

namespace veyra::ipc {

bool readMessage(HANDLE pipe, Message& out)
{
    // 64 KiB is comfortably larger than any Phase 1 message; ERROR_MORE_DATA
    // handles the rare case (e.g. a very large config) by accumulating.
    char buffer[64 * 1024];
    std::string bytes;

    for (;;)
    {
        DWORD read = 0;
        const BOOL ok = ReadFile(pipe, buffer, sizeof(buffer), &read, nullptr);
        if (ok)
        {
            bytes.append(buffer, read);
            break;
        }

        if (GetLastError() == ERROR_MORE_DATA)
        {
            bytes.append(buffer, read);
            continue;
        }
        return false; // ERROR_BROKEN_PIPE, ERROR_OPERATION_ABORTED, etc.
    }

    return parse(bytes, out);
}

bool writeMessage(HANDLE pipe, const Message& msg)
{
    const std::string bytes = serialize(msg);
    DWORD written = 0;
    const BOOL ok = WriteFile(pipe, bytes.data(), static_cast<DWORD>(bytes.size()),
                              &written, nullptr);
    return ok && written == bytes.size();
}

} // namespace veyra::ipc
