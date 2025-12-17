#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUSES 5
#define MAX_ROUTE_LEN 50
#define DEFAULT_CAPACITY 30
#define BASE_FARE_PER_KM 2.0f
#define STATE_FILE "buses_state.txt"
#define BOOKINGS_FILE "bookings.txt"

typedef struct {
    int id;
    char departure_city[MAX_ROUTE_LEN];
    char arrival_city[MAX_ROUTE_LEN];
    char via_stop[MAX_ROUTE_LEN];
    char departure_time[6];
    char arrival_time[6];
    int total_capacity;
    int current_passengers;
    int distance_km;
    float fare;
    int break_time_min;
} Bus;

typedef struct {
    char *name;
    int age;
    char *gender;
} Passenger;

Bus buses[MAX_BUSES];
int next_bus_id = 101;

static void clear_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

int time_to_minutes(const char *time_str) {
    int hour, minute;
    if (sscanf(time_str, "%d:%d", &hour, &minute) != 2) return -1;
    return hour * 60 + minute;
}

int calculate_duration(const char *dep_time, const char *arr_time) {
    int dep = time_to_minutes(dep_time);
    int arr = time_to_minutes(arr_time);
    if (dep == -1 || arr == -1) return 0;
    int dur = arr - dep;
    if (dur < 0) dur += 24 * 60;
    return dur;
}

int calculate_break_time(int distance) {
    if (distance <= 200) return 10;
    else if (distance <= 400) return 20;
    else if (distance <= 700) return 30;
    else return 45;
}

/* Persistence */
void save_bus_state() {
    FILE *f = fopen(STATE_FILE, "w");
    if (!f) return;
    for (int i = 0; i < MAX_BUSES; ++i) fprintf(f, "%d %d\n", buses[i].id, buses[i].current_passengers);
    fclose(f);
}

void load_bus_state() {
    FILE *f = fopen(STATE_FILE, "r");
    if (!f) return;
    int id, cur;
    while (fscanf(f, "%d %d", &id, &cur) == 2) {
        for (int i = 0; i < MAX_BUSES; ++i) {
            if (buses[i].id == id) {
                if (cur >= 0 && cur <= buses[i].total_capacity) buses[i].current_passengers = cur;
                break;
            }
        }
    }
    fclose(f);
}

void append_booking_record(const char *route_summary, Passenger *plist, int pcount, float total_fare, const char *payment_mode, const char *payment_ref) {
    FILE *f = fopen(BOOKINGS_FILE, "a");
    if (!f) return;
    fprintf(f, "----- Booking -----\n");
    fprintf(f, "Route: %s\n", route_summary);
    fprintf(f, "Passengers: %d\n", pcount);
    for (int i = 0; i < pcount; ++i) fprintf(f, "  %d) Name: %s | Age: %d | Gender: %s\n", i+1, plist[i].name, plist[i].age, plist[i].gender);
    fprintf(f, "Total Fare: ₹%.2f\n", total_fare);
    fprintf(f, "Payment: %s (%s)\n", payment_mode, payment_ref ? payment_ref : "N/A");
    fprintf(f, "-------------------\n\n");
    fclose(f);
}

/* Initialization */
void initialize_buses() {
    buses[0] = (Bus){ next_bus_id++, "New Delhi", "Agra", "Mathura", "06:00", "10:00", DEFAULT_CAPACITY, 0, 230, 230 * BASE_FARE_PER_KM, calculate_break_time(230) };
    buses[1] = (Bus){ next_bus_id++, "New Delhi", "Jaipur", "Gurugram", "08:30", "14:00", DEFAULT_CAPACITY, 0, 280, 280 * BASE_FARE_PER_KM, calculate_break_time(280) };
    buses[2] = (Bus){ next_bus_id++, "New Delhi", "Kanpur", "Aligarh", "09:15", "18:00", DEFAULT_CAPACITY, 0, 440, 440 * BASE_FARE_PER_KM, calculate_break_time(440) };
    buses[3] = (Bus){ next_bus_id++, "New Delhi", "Varanasi", "Prayagraj", "17:00", "07:00", DEFAULT_CAPACITY, 0, 820, 820 * BASE_FARE_PER_KM, calculate_break_time(820) };
    buses[4] = (Bus){ next_bus_id++, "New Delhi", "Chandigarh", "Panipat", "07:00", "12:00", DEFAULT_CAPACITY, 0, 250, 250 * BASE_FARE_PER_KM, calculate_break_time(250) };
    load_bus_state();
}

/* Display */
void show_all_routes() {
    printf("\nAvailable Routes from New Delhi:\n");
    printf("-----------------------------------------------------------------------------------------\n");
    printf("ID  | Destination      | Dep   | Arr   | Dist(km) | Fare(₹)  | Seats Left | Via Stop\n");
    printf("-----------------------------------------------------------------------------------------\n");
    for (int i = 0; i < MAX_BUSES; ++i) {
        int seats_left = buses[i].total_capacity - buses[i].current_passengers;
        printf("%-3d | %-15s | %-5s | %-5s | %-8d | %-8.2f | %-10d | %s\n",
               buses[i].id, buses[i].arrival_city, buses[i].departure_time, buses[i].arrival_time,
               buses[i].distance_km, buses[i].fare, seats_left, buses[i].via_stop);
    }
    printf("-----------------------------------------------------------------------------------------\n");
}

void display_schedules(const char *dep_city, const char *arr_city) {
    int found = 0;
    printf("\n=========================================================================================\n");
    printf("BUS SCHEDULE: %s -> %s\n", dep_city, arr_city);
    printf("=========================================================================================\n");
    printf("ID  | Dep   | Arr   | Duration | Seats Left | Distance | Break | Fare(₹)  | Via Stop\n");
    printf("----------------------------------------------------------------------------------------\n");
    for (int i = 0; i < MAX_BUSES; ++i) {
        if (strcmp(buses[i].departure_city, dep_city) == 0 && strcmp(buses[i].arrival_city, arr_city) == 0) {
            found = 1;
            int dur = calculate_duration(buses[i].departure_time, buses[i].arrival_time);
            int h = dur / 60, m = dur % 60;
            int seats_left = buses[i].total_capacity - buses[i].current_passengers;
            printf("%-3d | %-5s  | %-5s  | %2dh %02dm  | %-10d | %-8dkm | %-4dmin | %-8.2f | %s\n",
                   buses[i].id, buses[i].departure_time, buses[i].arrival_time, h, m, seats_left,
                   buses[i].distance_km, buses[i].break_time_min, buses[i].fare, buses[i].via_stop);
        }
    }
    if (!found) printf("No routes found for %s to %s.\n", dep_city, arr_city);
    printf("=========================================================================================\n");
}

/* Input helper */
char *read_line_alloc(size_t maxlen) {
    char buffer[512];
    if (!fgets(buffer, (int)sizeof(buffer), stdin)) return NULL;
    size_t len = strcspn(buffer, "\n");
    buffer[len] = '\0';
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    strcpy(out, buffer);
    return out;
}

/* Payment */
int process_payment_simulation(float amount, char *payment_ref, size_t ref_len) {
    printf("\n--- Payment ---\n");
    printf("Total amount to pay: ₹%.2f\n", amount);
    printf("1. UPI (Pay to 9257797493-2@ybl)\n");
    printf("2. Credit/Debit Card (enter last 4 digits)\n");
    printf("Choose payment method (1 or 2): ");
    int opt;
    if (scanf("%d", &opt) != 1) { clear_stdin(); printf("Invalid input.\n"); return 0; }
    clear_stdin();
    if (opt == 1) {
        printf("Please send ₹%.2f to UPI ID: 9257797493-2@ybl\n", amount);
        printf("After payment, type 'yes' to confirm: ");
        char *resp = read_line_alloc(32);
        int ok = 0;
        if (resp) {
            if (strcmp(resp, "yes") == 0 || strcmp(resp, "YES") == 0 || strcmp(resp, "Yes") == 0) ok = 1;
            free(resp);
        }
        if (payment_ref && ref_len > 0) snprintf(payment_ref, ref_len, "UPI:9257797493-2@ybl");
        if (ok) { printf("Payment verified.\n"); return 1; } else { printf("Payment not confirmed.\n"); return 0; }
    } else if (opt == 2) {
        printf("Enter last 4 digits of your card: ");
        char *card = read_line_alloc(8);
        if (!card) return 0;
        if (payment_ref && ref_len > 0) snprintf(payment_ref, ref_len, "CARD:%s", card);
        printf("Processing card ending with %s ...\n", card);
        printf("Payment successful.\n");
        free(card);
        return 1;
    } else {
        printf("Invalid payment option.\n");
        return 0;
    }
}

/* Stops */
void show_stops(const Bus *bus) {
    printf("\nRoute stops for Bus %d:\n", bus->id);
    printf("1) %s (Departure)\n", bus->departure_city);
    printf("2) %s (Via stop)\n", bus->via_stop);
    printf("3) %s (Destination)\n", bus->arrival_city);
}

/* Booking (multiple passengers) */
void book_transport_multi() {
    show_all_routes();
    printf("\nEnter Bus ID to book: ");
    int bus_id;
    if (scanf("%d", &bus_id) != 1) { clear_stdin(); printf("Invalid input.\n"); return; }
    clear_stdin();
    Bus *bus = NULL;
    for (int i = 0; i < MAX_BUSES; ++i) if (buses[i].id == bus_id) { bus = &buses[i]; break; }
    if (!bus) { printf("Bus not found.\n"); return; }
    int seats_left = bus->total_capacity - bus->current_passengers;
    if (seats_left <= 0) { printf("Sorry, this bus is full.\n"); return; }
    show_stops(bus);
    printf("\nHow many passengers do you want to book (max %d)? ", seats_left);
    int pcount;
    if (scanf("%d", &pcount) != 1) { clear_stdin(); printf("Invalid number.\n"); return; }
    clear_stdin();
    if (pcount <= 0) { printf("Must be at least 1 passenger.\n"); return; }
    if (pcount > seats_left) { printf("Only %d seats left. Reduce passenger count.\n", seats_left); return; }
    Passenger *plist = (Passenger *)malloc(sizeof(Passenger) * pcount);
    if (!plist) { printf("Memory allocation failed.\n"); return; }
    memset(plist, 0, sizeof(Passenger) * pcount);
    for (int i = 0; i < pcount; ++i) {
        printf("\nPassenger %d details:\n", i + 1);
        printf("Name: ");
        plist[i].name = read_line_alloc(100);
        if (!plist[i].name) plist[i].name = strdup("Unknown");
        printf("Age: ");
        if (scanf("%d", &plist[i].age) != 1) { clear_stdin(); plist[i].age = 0; }
        clear_stdin();
        printf("Gender (M/F/O): ");
        plist[i].gender = read_line_alloc(10);
        if (!plist[i].gender) plist[i].gender = strdup("U");
    }
    float total_fare = bus->fare * (float)pcount;
    printf("\nBooking Summary:\n");
    printf("Route: %s -> %s (via %s)\n", bus->departure_city, bus->arrival_city, bus->via_stop);
    printf("Departure: %s | Arrival: %s | Distance: %dkm | Break: %d min\n", bus->departure_time, bus->arrival_time, bus->distance_km, bus->break_time_min);
    printf("Passengers: %d | Total Fare: ₹%.2f\n", pcount, total_fare);
    printf("Seats available before booking: %d\n", seats_left);
    printf("\nDo you want to proceed to payment and confirm booking? (Y/N): ");
    char *resp = read_line_alloc(8);
    if (!resp || (resp[0] != 'Y' && resp[0] != 'y')) {
        printf("Booking cancelled by user.\n");
        for (int i = 0; i < pcount; ++i) { free(plist[i].name); free(plist[i].gender); }
        free(plist);
        if (resp) free(resp);
        return;
    }
    if (resp) free(resp);
    char payment_ref[64] = {0};
    int paid = process_payment_simulation(total_fare, payment_ref, sizeof(payment_ref));
    if (!paid) {
        printf("Payment failed or not confirmed. Booking not completed.\n");
        for (int i = 0; i < pcount; ++i) { free(plist[i].name); free(plist[i].gender); }
        free(plist);
        return;
    }
    for (int i = 0; i < pcount; ++i) bus->current_passengers++;
    seats_left = bus->total_capacity - bus->current_passengers;
    save_bus_state();
    printf("\n\n==== Booking Confirmed ====\n");
    printf("Bus ID: %d | Route: %s -> %s (via %s)\n", bus->id, bus->departure_city, bus->arrival_city, bus->via_stop);
    printf("Departure: %s | Arrival: %s\n", bus->departure_time, bus->arrival_time);
    printf("Break Time: %d min | Distance: %dkm\n", bus->break_time_min, bus->distance_km);
    printf("Passengers (%d):\n", pcount);
    for (int i = 0; i < pcount; ++i) printf("  %d) %s, Age: %d, Gender: %s\n", i+1, plist[i].name, plist[i].age, plist[i].gender);
    printf("Total Paid: ₹%.2f via %s\n", total_fare, payment_ref[0] ? payment_ref : "N/A");
    printf("Seats left after booking: %d\n", seats_left);
    printf("===========================\n");
    char route_summary[200];
    snprintf(route_summary, sizeof(route_summary), "%s -> %s (Bus %d)", bus->departure_city, bus->arrival_city, bus->id);
    append_booking_record(route_summary, plist, pcount, total_fare, payment_ref[0] ? (strstr(payment_ref, "UPI") ? "UPI" : "CARD") : "UNKNOWN", payment_ref);
    for (int i = 0; i < pcount; ++i) { free(plist[i].name); free(plist[i].gender); }
    free(plist);
}

/* Main */
int main(void) {
    initialize_buses();
    int choice = 0;
    char dest[MAX_ROUTE_LEN];
    printf("Welcome to MoveMate - Intelligent Transport Booking (Console Demo)\n");
    do {
        printf("\nMain Menu\n");
        printf("1) View All Routes\n");
        printf("2) View Schedule of a Route (with breaks & via stops)\n");
        printf("3) Book Seats (multiple passengers supported)\n");
        printf("4) Exit\n");
        printf("Enter choice: ");
        if (scanf("%d", &choice) != 1) { clear_stdin(); printf("Invalid input. Try again.\n"); continue; }
        clear_stdin();
        switch (choice) {
            case 1: show_all_routes(); break;
            case 2:
                printf("\nEnter Destination City (Agra/Jaipur/Kanpur/Varanasi/Chandigarh): ");
                if (fgets(dest, sizeof(dest), stdin) != NULL) { dest[strcspn(dest, "\n")] = '\0'; display_schedules("New Delhi", dest); }
                break;
            case 3: book_transport_multi(); break;
            case 4: printf("Saving state and exiting. Goodbye!\n"); save_bus_state(); break;
            default: printf("Invalid option. Choose 1-4.\n"); break;
        }
    } while (choice != 4);
    return 0;
}