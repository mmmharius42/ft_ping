NAME			= ft_ping
CC				= gcc
CFLAGS			= -Wall -Wextra -Werror
CPPFLAGS		= -I.

SRCS_DIR		= src
OBJS_DIR		= objs

SRCS			= $(SRCS_DIR)/ft_ping.c

OBJS			= $(SRCS:$(SRCS_DIR)/%.c=$(OBJS_DIR)/%.o)

DEPS			= $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c
	@mkdir -p $(OBJS_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -c $< -o $@

clean:
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re