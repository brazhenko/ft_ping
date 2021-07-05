# **************************************************************************** #
# GENERIC_VARIABLES

OBJ_DIR = build

# **************************************************************************** #
# COMPILER_OPTIONS

C_COMPILER = gcc
C_STANDART =
C_CFLAGS =  $(CFLAGS) $(CPPFLAGS) -Wall -Wextra -Werror
C_LFLAGS =  $(CFLAGS) $(CPPFLAGS) -Wall -Wextra -Werror

# **************************************************************************** #
# FT_PING TARGET DESCRIPTION

FT_PING_NAME = ft_ping
FT_PING_PATH = .
FT_PING_FILE = build/ft_ping
FT_PING_SRCS = dns_resolvers.c initialize_context.c initialize_signals.c main.c ping_io.c send_icmp_msg_v4.c
FT_PING_OBJS = $(patsubst %, $(OBJ_DIR)/%.o, $(FT_PING_SRCS))
FT_PING_DEPS = $(patsubst %, $(OBJ_DIR)/%.d, $(FT_PING_SRCS))
FT_PING_LIBS =
FT_PING_INCS =

# **************************************************************************** #
# GENERIC RULES

.PHONY: all re clean fclean
.DEFAULT_GOAL = all

all:  $(FT_PING_FILE)

clean:
	@rm -rf $(OBJ_DIR)

fclean: clean
	@rm -rf  $(FT_PING_FILE)

re: fclean all

$(FT_PING_FILE):  $(FT_PING_OBJS)
	@$(C_COMPILER) $(C_LFLAGS) $(C_STANDART) -o $(FT_PING_FILE) $(FT_PING_OBJS)  $(FT_PING_LIBS) -lm -lpthread
	@printf 'Finished	\033[1;32m\033[7m$@ \033[0m\n\n'

$(OBJ_DIR)/%.c.o: $(FT_PING_PATH)/%.c
	@mkdir -p $(OBJ_DIR)
	@printf 'Compiling	\033[1;33m$<\033[0m ...\n'
	@$(C_COMPILER) $(C_CFLAGS) $(C_STANDART) $(FT_PING_INCS) -o $@ -c $< -MMD

-include $(FT_PING_DEPS)