NAME = FTVM
##========== SOURCES ==========##

SRC  := Logger.cpp \
	FTVM.cpp \
	utils.cpp \
	sha256.cpp
MAIN := main.cpp

##========== NAMES ==========##

NAME = FTVM
SRCS_DIR = ./src/
OBJS_DIR = ./OBJ/

##========== OBJECTS ==========##

OBJS := $(addprefix $(OBJS_DIR),$(SRC:.cpp=.o))
MAIN_OBJ = $(addprefix $(OBJS_DIR),$(MAIN:.cpp=.o))

##========== COLORS ==========##

BASE_COLOR 	=		\033[0;39m
GRAY 		=		\033[0;90m
RED 		=		\033[0;91m
GREEN 		=		\033[0;92m
YELLOW 		=		\033[0;93m
BLUE 		=		\033[0;94m
MAGENTA		=		\033[0;95m
CYAN 		=		\033[0;96m
WHITE		=		\033[0;97m

##========== COMPILATOR ==========##

CXX = c++

##========== FLAGS ==========##

CXXFLAGS = -Wall -Wextra -std=c++98 -Iincludes -fnon-call-exceptions
LDFLAGS = $(LIBS) -fnon-call-exceptions
LIBS = 

ifdef ERRORED
	CXXFLAGS += -Werror
endif

##========== MODES ==========##

ifdef DEBUG
	CXXFLAGS += -g
	#LDFLAGS += -fsanitize=thread #(address, memory, thread)
	DEBUG_MODE = 1
endif

##========== ANIMATIONS ==========##

NUM_SRC = $(words $(SRC))
INDEX = 0
NUM_LINES_TO_CLEAR = 1

all : $(CLEAR) $(NAME)

$(NAME) : $(OBJS) $(MAIN_OBJ)
	@$(CXX) -o $(NAME) $(CXXFLAGS) $(MAIN_OBJ) $(OBJS) $(LDFLAGS)
	@echo "$(GREEN)-= $(NAME) compiled =-$(BASE_COLOR)"

clean :
	@rm -rf $(OBJS_DIR)

fclean : clean
	@rm -rf $(NAME)
	@echo "$(CYAN)Files cleaned$(BASE_COLOR)"

re : fclean all

$(OBJS_DIR)%.o : $(SRCS_DIR)%.cpp
	@echo "Compiling : $<"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY : all bonus clean fclean re
