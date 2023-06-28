CC = gcc -std=c11
CPPC = g++ -std=c++17
LDFLAGS = -pthread
BLDDIR = .
INCDIR = $(BLDDIR)/inc
SRCDIR = $(BLDDIR)/src
OBJDIR = $(BLDDIR)/obj
CFLAGS = -c -Wall -Wextra -pedantic -I$(INCDIR)
SRCCPP = $(wildcard $(SRCDIR)/*.cpp)
OBJCPP = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCCPP))
SRCC = $(wildcard $(SRCDIR)/*.c)
OBJC = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCC))
OBJ = $(OBJC) $(OBJCPP) 
EXE = bin

all: clean $(EXE)
    
$(EXE) : $(OBJ) 
	$(CPPC) $(OBJDIR)/*.o -o $@ $(LDFLAGS)

$(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CPPC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(OBJDIR)/%.o : $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	-rm -f ./$(OBJDIR)/*.o $(EXE)