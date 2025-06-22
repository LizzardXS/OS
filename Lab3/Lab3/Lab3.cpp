#include <iostream>  
#include <vector>  
#include <windows.h>  
#include <string>     
#include <cstdlib>    
#include <ctime>      


std::vector<int> g_array;               
int g_array_size = 0;                   
int g_active_markers_count = 0;       


CRITICAL_SECTION g_array_cs;       
CRITICAL_SECTION g_console_cs;      
CRITICAL_SECTION g_active_markers_cs;  

std::vector<HANDLE> g_marker_stuck_events; 
std::vector<HANDLE> g_marker_continue_terminate_events; 

HANDLE g_start_all_markers_event;     

// --- Structure to pass data to marker threads ---
struct MarkerThreadData {
    int id;                           
    std::vector<int> marked_indices;  
    HANDLE thread_handle;             
};


DWORD WINAPI marker(LPVOID lpParam) {
    MarkerThreadData* data = static_cast<MarkerThreadData*>(lpParam);
    
    // Wait for the main thread's signal to begin work.
    WaitForSingleObject(g_start_all_markers_event, INFINITE);

    srand(data->id + static_cast<unsigned int>(time(NULL))); 

    int elements_marked_by_this_thread = 0; 

    // Main working loop for the marker thread.
    while (true) {
        int random_index = rand() % g_array_size; 

        // Protect access to the shared array.
        EnterCriticalSection(&g_array_cs); 
        
        // If the array element is 0 (unmarked), attempt to mark it.
        if (g_array[random_index] == 0) {
            LeaveCriticalSection(&g_array_cs); 
            Sleep(5); 

            EnterCriticalSection(&g_array_cs); 
            if (g_array[random_index] == 0) { 
                g_array[random_index] = data->id; 
                data->marked_indices.push_back(random_index);
                elements_marked_by_this_thread++;
                LeaveCriticalSection(&g_array_cs); 
                Sleep(5); 
            } else { // Element was marked by another thread while this thread was sleeping.
                LeaveCriticalSection(&g_array_cs); // Release lock.
                // Fall through to "stuck" logic.
                goto stuck_label;
            }
        } else { 
            LeaveCriticalSection(&g_array_cs); 
            
        stuck_label: 
            EnterCriticalSection(&g_console_cs); 
            std::cout << "Marker " << data->id 
                      << ": Cannot mark element (already taken). Marked by this thread: " 
                      << elements_marked_by_this_thread << ", Index: " << random_index << "\n";
            LeaveCriticalSection(&g_console_cs);

            // Signal main thread that this marker is stuck.
            SetEvent(g_marker_stuck_events[data->id - 1]); 
            
            // Wait for main thread's signal to continue or terminate.
            DWORD wait_result = WaitForSingleObject(g_marker_continue_terminate_events[data->id - 1], INFINITE);

            // If the signal indicates termination, break loop to clean up.
            if (wait_result != WAIT_OBJECT_0) { 
                break; // Exit loop for thread termination.
            }
        }
    }

    // --- Thread Termination Logic ---
    // Fill all elements marked by this thread with zeros.
    EnterCriticalSection(&g_array_cs); 
    for (int index : data->marked_indices) {
        if (g_array[index] == data->id) { // Only clear if this marker still owns it.
            g_array[index] = 0; 
        }
    }
    LeaveCriticalSection(&g_array_cs);

    // Notify console that this marker is finishing.
    EnterCriticalSection(&g_console_cs);
    std::cout << "Marker " << data->id << " is terminating and clearing its marks.\n";
    LeaveCriticalSection(&g_console_cs);

    // Decrement the global active marker count.
    EnterCriticalSection(&g_active_markers_cs);
    g_active_markers_count--;
    LeaveCriticalSection(&g_active_markers_cs);

    return 0; // Thread exits.
}

// --- Main Function (Main Thread) ---
int main() {
    // Initialize critical sections.
    InitializeCriticalSection(&g_array_cs);
    InitializeCriticalSection(&g_console_cs);
    InitializeCriticalSection(&g_active_markers_cs);

    // Get array size from user and initialize array with zeros.
    std::cout << "Enter array size: ";
    std::cin >> g_array_size;
    g_array.resize(g_array_size, 0); 

    // Get number of marker threads to create.
    int num_markers;
    std::cout << "Enter number of marker threads: ";
    std::cin >> num_markers;

    std::vector<MarkerThreadData> marker_data(num_markers);
    std::vector<HANDLE> marker_thread_handles(num_markers);
    g_marker_stuck_events.resize(num_markers);
    g_marker_continue_terminate_events.resize(num_markers);

    g_active_markers_count = num_markers; 

    g_start_all_markers_event = CreateEvent(NULL, TRUE, FALSE, NULL); 
    g_all_markers_stuck_event = CreateEvent(NULL, TRUE, FALSE, NULL); 

    // Create each marker thread and their associated events.
    for (int i = 0; i < num_markers; ++i) {
        marker_data[i].id = i + 1; 
        marker_data[i].thread_handle = NULL; 

        // Create specific events for this marker thread's communication.
        g_marker_stuck_events[i] = CreateEvent(NULL, FALSE, FALSE, NULL); // Auto-reset, non-signaled
        g_marker_continue_terminate_events[i] = CreateEvent(NULL, FALSE, FALSE, NULL); // Auto-reset, non-signaled

        // Create the thread, passing its data.
        marker_thread_handles[i] = CreateThread(
            NULL, 0, marker, &marker_data[i], 0, NULL);

        if (marker_thread_handles[i] == NULL) {
            std::cerr << "Error creating marker thread " << i + 1 << "\n";
            return 1;
        }
        marker_data[i].thread_handle = marker_thread_handles[i]; // Store handle for main's use.
    }

    // Signal all marker threads to begin their work.
    std::cout << "\nMain: Signaling all marker threads to start...\n";
    SetEvent(g_start_all_markers_event);

    // --- Main coordination loop ---
    while (g_active_markers_count > 0) {

        
        std::vector<HANDLE> active_stuck_events_copy;
        for (int i = 0; i < num_markers; ++i) {
             if (marker_thread_handles[i] != NULL) { // Only include events for active threads.
                active_stuck_events_copy.push_back(g_marker_stuck_events[i]);
            }
        }

        if (!active_stuck_events_copy.empty()) {
            std::cout << "\nMain: Waiting for " << active_stuck_events_copy.size() << " active markers to get 'stuck'...\n";
            DWORD wait_all_result = WaitForMultipleObjects(active_stuck_events_copy.size(), active_stuck_events_copy.data(), TRUE, INFINITE);
            if (wait_all_result == WAIT_FAILED) {
                std::cerr << "Main: WaitForMultipleObjects failed: " << GetLastError() << "\n";
                break;
            }
        }
       
        EnterCriticalSection(&g_console_cs);
        std::cout << "\nMain: All active marker threads are 'stuck'.\n";
        LeaveCriticalSection(&g_console_cs);

        // Display the current state of the array.
        EnterCriticalSection(&g_console_cs);
        std::cout << "\nCurrent Array State:\n";
        for (int i = 0; i < g_array_size; ++i) {
            std::cout << g_array[i] << " ";
        }
        std::cout << "\n";
        LeaveCriticalSection(&g_console_cs);

        if (g_active_markers_count == 0) { 
            std::cout << "Main: All markers have finished, exiting coordination loop.\n";
            break;
        }

        // Prompt user to select a marker thread to terminate.
        int marker_to_terminate_id = -1;
        bool valid_id_entered = false;
        while (!valid_id_entered) {
            std::cout << "Enter ID of marker thread to terminate (1-" << num_markers << "): ";
            std::cin >> marker_to_terminate_id;

            if (marker_to_terminate_id >= 1 && marker_to_terminate_id <= num_markers) {
                if (marker_thread_handles[marker_to_terminate_id - 1] != NULL) { // Check if thread is still active.
                    valid_id_entered = true;
                } else {
                    std::cout << "Marker thread " << marker_to_terminate_id << " already terminated. Choose another.\n";
                }
            } else {
                std::cout << "Invalid ID. Please enter a number between 1 and " << num_markers << ".\n";
            }
        }

        // Signal the selected marker thread to terminate.
        std::cout << "Main: Signaling marker thread " << marker_to_terminate_id << " to terminate.\n";
        SetEvent(g_marker_continue_terminate_events[marker_to_terminate_id - 1]);

        // Wait for the selected marker thread to fully terminate.
        std::cout << "Main: Waiting for marker thread " << marker_to_terminate_id << " to finish...\n";
        WaitForSingleObject(marker_thread_handles[marker_to_terminate_id - 1], INFINITE);
        
        // Close the handle of the terminated thread.
        CloseHandle(marker_thread_handles[marker_to_terminate_id - 1]);
        marker_thread_handles[marker_to_terminate_id - 1] = NULL; // Mark as inactive.

        EnterCriticalSection(&g_console_cs);
        std::cout << "Main: Marker thread " << marker_to_terminate_id << " has terminated.\n";
        LeaveCriticalSection(&g_console_cs);

        // Display array state after a thread terminates.
        EnterCriticalSection(&g_console_cs);
        std::cout << "\nArray State After Thread Termination:\n";
        for (int i = 0; i < g_array_size; ++i) {
            std::cout << g_array[i] << " ";
        }
        std::cout << "\n";
        LeaveCriticalSection(&g_console_cs);

        // Signal remaining active marker threads to continue working.
        if (g_active_markers_count > 0) {
            std::cout << "Main: Signaling remaining marker threads to continue.\n";
            for (int i = 0; i < num_markers; ++i) {
                if (marker_thread_handles[i] != NULL) { // Only signal active threads.
                    SetEvent(g_marker_continue_terminate_events[i]);
                }
            }
        } else {
            std::cout << "Main: All markers have terminated. Exiting coordination loop.\n";
        }
    }

    std::cout << "\nMain: All marker threads have finished. Program exiting.\n";

    // Close all event handles.
    CloseHandle(g_start_all_markers_event);
    CloseHandle(g_all_markers_stuck_event);
    for (int i = 0; i < num_markers; ++i) {
        if (g_marker_stuck_events[i] != NULL) CloseHandle(g_marker_stuck_events[i]);
        if (g_marker_continue_terminate_events[i] != NULL) CloseHandle(g_marker_continue_terminate_events[i]);
    }

    // Delete critical sections.
    DeleteCriticalSection(&g_array_cs);
    DeleteCriticalSection(&g_console_cs);
    DeleteCriticalSection(&g_active_markers_cs);

    return 0;
}
