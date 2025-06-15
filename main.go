package main

/*
#include <stdio.h>
#include <stdlib.h>

// Function declaration that will call back into Go
extern char* getUserAddress(void* userHandle);

// C function that uses the Go User object
static void printUserAddress(void* userHandle) {
    char* address = getUserAddress(userHandle);
    printf("User's address from C: %s\n", address);
    free(address); // Free the C string allocated by Go
}
*/
import "C"
import (
	"runtime/cgo"
	"unsafe"
)

// User is our Go struct with methods
type User struct {
	Name    string
	Address string
}

// GetAddress is a method on User
func (u *User) GetAddress() string {
	return u.Address
}

//export getUserAddress
func getUserAddress(userHandle unsafe.Pointer) *C.char {
	// Retrieve the User object from the handle
	h := cgo.Handle(userHandle)
	user := h.Value().(*User)

	// Call the GetAddress method
	address := user.GetAddress()

	// Convert Go string to C string
	// Note: This allocates memory that must be freed by the C code
	return C.CString(address)
}

func main() {
	// Create a User instance
	user := &User{
		Name:    "John Doe",
		Address: "123 Main St, Anytown, USA",
	}

	// Create a handle to pass to C
	handle := cgo.NewHandle(user)
	defer handle.Delete() // Clean up when done

	// Call the C function, passing the handle
	C.printUserAddress(unsafe.Pointer(handle))
}
