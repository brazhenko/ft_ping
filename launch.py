from package import generate_makefile

PING = {
    "name": "ft_ping",
    "out": ".",
    "type": "prog",
    "path": ".",
    "sources": [
        "dns_resolvers.c",
        "initialize_context.c",
        "initialize_signals.c",
        "main.c",
        "ping_io.c",
        "send_icmp_msg_v4.c"
    ],
    "peerdirs": [],
    "includes": []
}

COMPILERS = {
    "c": {
        "flags": "-Wall -Wextra -Werror",
        "ldflags": "-lm -lpthread",
        "file_extension": "c",
        "binary": "gcc",
        "phtread": True
    }
}

generate_makefile(PING, COMPILERS)