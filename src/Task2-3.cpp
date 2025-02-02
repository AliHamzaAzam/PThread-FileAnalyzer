//
// Created by Ali Hamza Azam on 27/01/2025.
// ID : 22I-2126
// Parallel and Distributed Computing - Assignment : 1
//
#include "utility.h"
#include <regex>


mutex mtx;                                 // Mutex for critical sections

struct TextData {
    unordered_map<string, int> word_count; //words with frequency
    int num_words = 0;                     //total words
    int num_vowel_words = 0;               //total words starting with vowels
};

struct ThreadArgs {
    int thread_id;
    int core_id;
    const char* data;
    long start;
    long end;
    TextData* text_data;

    ThreadArgs(int thread_id, int core_id, const char* data, long start, long end, TextData* text_data)
            : thread_id(thread_id), core_id(core_id), data(data), start(start), end(end), text_data(text_data) {}
    ThreadArgs() = default;
};


// Helper function to check if a word starts with a vowel
bool starts_with_vowel(const string& word) {
    if (word.empty()) return false;
    char first_char = tolower(word[0]);
    return first_char == 'a' || first_char == 'e' || first_char == 'i' ||
           first_char == 'o' || first_char == 'u';
}

// Thread function to process a chunk of data
void* process_chunk(void* arg) {
    auto* args = static_cast<ThreadArgs*>(arg);

    if (args->core_id != -1) {
        set_thread_affinity(pthread_self(), args->core_id);
    }

    int local_word_count = 0;
    int local_vowel_word_count = 0;
    unordered_map<string, int> local_words;
    const char* data = args->data + args->start;
    const char* end = args->data + args->end;

    while (data < end) {
        // Find the end of the line
        const char* line_end = static_cast<const char*>(memchr(data, '\n', end - data));
        if (!line_end) {
            line_end = end;
        }

        // Extract the line
        string line(data, line_end - data);
        data = line_end + 1;
        if (line.empty()) {
            continue;
        }
        size_t start = 0;
        size_t space = line.find(' ');

        // Process each word in the line
        while (start < line.size()) {
            string word = line.substr(start, space - start); // Extract word

            // Move start to the next position after the space
            start = (space == string::npos) ? line.size() : space + 1;
            space = line.find(' ', start);

            // Skip empty words
            if (word.empty()) {
                continue;
            }

            // Remove any leading/trailing punctuation using regex
            static const regex word_regex(R"(\b(?=\w*[a-zA-Z'])[a-zA-Z0-9'-]+(?:-[a-zA-Z0-9'-]+)*\b)");
            smatch match;
            if (regex_search(word, match, word_regex)) {
                word = match.str(); // Only keep the matched alphabetic part
            } else {
                continue;           // Skip if no match found
            }


            transform(word.begin(), word.end(), word.begin(), ::tolower); // Convert to lowercase

            // Update local data
            local_word_count++;
            local_words[word]++;
            if (starts_with_vowel(word)) {
              local_vowel_word_count++;
            }

        }
    }

    // Update global data after processing the chunk
    {
        lock_guard<mutex> lock(mtx);
        args->text_data->num_words += local_word_count;
        args->text_data->num_vowel_words += local_vowel_word_count;
        for (const auto& [word, count] : local_words) {
            args->text_data->word_count[word] += count;
        }
    }
    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <file_path> <num_threads> [core_affinity] [save_to_file]\n";
        return 1;
    }

    string file_path = argv[1];
    int num_threads = stoi(argv[2]);
    if (num_threads <= 0) {
        cerr << "Invalid number of threads\n";
        return 1;
    }
    bool core_affinity = (argc > 3) ? (string(argv[3]) == "true") : false;
    bool save_to_file = (argc > 4) ? (string(argv[4]) == "true") : false;

    // Pre-processing - START
    // Memory map file
    size_t length;
    void* mapped_data = mmap_file(file_path, length);
    if (!mapped_data) {
        return 1;
    }

    // Calculate chunk offsets
    auto offsets = calculate_chunk_offsets_mapped(static_cast<const char*>(mapped_data), length, num_threads);

    // Create threads
    pthread_t threads[num_threads];
    ThreadArgs thread_args[num_threads];
    TextData text_data;
    priority_queue<pair<int, string>, vector<pair<int, string>>, greater<>> top_words; // Min-heap for top 10
    for (int i = 0; i < num_threads; i++) {
        if (core_affinity) {
            thread_args[i] = ThreadArgs(i, i % CORE_COUNT, static_cast<const char*>(mapped_data), offsets[i], offsets[i + 1], &text_data);
        } else {
            thread_args[i] = ThreadArgs(i, -1, static_cast<const char*>(mapped_data), offsets[i], offsets[i + 1], &text_data);
        }
    }
    // Pre-processing - END


    // Main processing - START
    // Start threads
    auto start_time = chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], nullptr, process_chunk, &thread_args[i]);
    }

    // Join threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], nullptr);
    }
    auto join_time = chrono::high_resolution_clock::now();

    // Main processing - END

    // Post-processing - START
    // Determine top-10 words by frequency using a min-heap
    for (auto& [word, count] : text_data.word_count) {
        top_words.push({count, word});
        if (top_words.size() > 10) {
            top_words.pop();
        } else if (count > top_words.top().first) {
            top_words.pop();
            top_words.emplace(count, word);
        }
    }

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end_time - start_time;
    chrono::duration<double> join_elapsed = join_time - start_time;

    cout << "Execution Time: " << elapsed.count() << " seconds\n";
    cout << "Join time: "<< join_elapsed.count() << " seconds\n";

    // Output the results
    vector<pair<int, string>> top_words_vec;
    cout << "Total unique words: " << text_data.word_count.size() << endl;
    cout << "Total words starting with vowels: " << text_data.num_vowel_words << endl;
    cout << "Total words: " << text_data.num_words << endl;
    // Top 10 words
    cout << "Top 10 words with highest frequency:" << endl;
    while (!top_words.empty()) {
        cout << top_words.top().second << ": " << top_words.top().first << " occurrences" << endl;
        top_words_vec.push_back(top_words.top());
        top_words.pop();
    }
    /* Commented out as outout is too large
    // All unique words with frequency
    cout << "All unique words with frequency:" << endl;
    for (const auto& [word, count] : word_data.word_count) {
        cout << word << ": " << count << " occurrences" << endl;
    }
     */
    if (!save_to_file) {
        munmap(mapped_data, length);
        return 0;
    }
    // Save the results to ../output/task2-3_output.txt
    ofstream output_file("../output/task2-3_output.txt");
    output_file << "Total unique words: " << text_data.word_count.size() << endl;
    output_file << "Total words starting with vowels: " << text_data.num_vowel_words << endl;
    output_file << "Total words: " << text_data.num_words << endl;
    output_file << "Top 10 words with highest frequency:" << endl;
    while (!top_words_vec.empty()) {
        output_file << top_words_vec.back().second << ": " << top_words_vec.back().first << " occurrences" << endl;
        top_words_vec.pop_back();
    }
    for (const auto& [word, count] : text_data.word_count) {
        output_file << word << ": " << count << " occurrences" << endl;
    }
    output_file.close();
    // Post-processing - END

    // Free memory
    munmap(mapped_data, length);
    return 0;
}