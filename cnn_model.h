#ifndef CNN_MODEL_H
#define CNN_MODEL_H

#include <vector>
#include <string>
#include <fstream>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <random>

const int IMAGE_SIZE = 64;
const int NUM_CLASSES = 4;

struct Image {
    float data[IMAGE_SIZE][IMAGE_SIZE];
    int label;
};

class CNN {
public:
    CNN() {
        initialize_weights();
    }

    void train(const std::vector<Image>& train_data, int epochs, float learning_rate, size_t batch_size = 32) {
        std::cout << "Starting training with " << train_data.size() << " images." << std::endl;
        for (int epoch = 0; epoch < epochs; ++epoch) {
            for (size_t i = 0; i < train_data.size(); i += batch_size) {
                size_t end = std::min(i + batch_size, train_data.size());
                std::vector<Image> batch(train_data.begin() + i, train_data.begin() + end);
                augment_data(batch); // Apply data augmentation
                train_batch(batch, learning_rate);
            }
            std::cout << "Epoch " << epoch + 1 << " completed." << std::endl;
        }
    }

    int predict(const Image& img) {
        auto output = forward(img.data);
        int prediction = std::distance(output.begin(), std::max_element(output.begin(), output.end()));
        std::cout << "Predicted label: " << prediction << ", Actual label: " << img.label << std::endl;
        return prediction;
    }

    void save_weights(const std::string& file_path) {
        std::ofstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file to save weights." << std::endl;
            return;
        }
        save_vector(file, conv1_weights);
        save_vector(file, conv2_weights);
        save_vector(file, conv3_weights);
        save_vector(file, fc1_weights);
        save_vector(file, fc2_weights);
        save_vector(file, fc3_weights);
        file.close();
    }

    void load_weights(const std::string& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file to load weights." << std::endl;
            return;
        }
        load_vector(file, conv1_weights);
        load_vector(file, conv2_weights);
        load_vector(file, conv3_weights);
        load_vector(file, fc1_weights);
        load_vector(file, fc2_weights);
        load_vector(file, fc3_weights);
        file.close();
    }

private:
    std::vector<std::vector<float>> conv1_weights;
    std::vector<std::vector<float>> conv2_weights;
    std::vector<std::vector<float>> conv3_weights;
    std::vector<float> fc1_weights;
    std::vector<float> fc2_weights;
    std::vector<float> fc3_weights;

    void initialize_weights() {
        conv1_weights = std::vector<std::vector<float>>(32, std::vector<float>(9, 0.01f));
        conv2_weights = std::vector<std::vector<float>>(64, std::vector<float>(32 * 3 * 3, 0.01f));
        conv3_weights = std::vector<std::vector<float>>(128, std::vector<float>(64 * 3 * 3, 0.01f));
        fc1_weights = std::vector<float>(128 * 8 * 8, 0.01f);
        fc2_weights = std::vector<float>(256, 0.01f);
        fc3_weights = std::vector<float>(128, 0.01f);
    }

    std::vector<float> forward(const float input[IMAGE_SIZE][IMAGE_SIZE]) {
        std::vector<float> conv_output1(32 * 32 * 32, 0.0f);
        for (int f = 0; f < 32; ++f) {
            for (int i = 0; i < 32; ++i) {
                for (int j = 0; j < 32; ++j) {
                    float sum = 0.0f;
                    for (int ki = 0; ki < 3; ++ki) {
                        for (int kj = 0; kj < 3; ++kj) {
                            sum += input[i + ki][j + kj] * conv1_weights[f][ki * 3 + kj];
                        }
                    }
                    conv_output1[f * 32 * 32 + i * 32 + j] = std::max(0.0f, sum);
                }
            }
        }

        std::vector<float> conv_output2(64 * 16 * 16, 0.0f);
        for (int f = 0; f < 64; ++f) {
            for (int i = 0; i < 16; ++i) {
                for (int j = 0; j < 16; ++j) {
                    float sum = 0.0f;
                    for (int ki = 0; ki < 3; ++ki) {
                        for (int kj = 0; kj < 3; ++kj) {
                            sum += conv_output1[(i + ki) * 32 + (j + kj)] * conv2_weights[f][ki * 3 + kj];
                        }
                    }
                    conv_output2[f * 16 * 16 + i * 16 + j] = std::max(0.0f, sum);
                }
            }
        }

        std::vector<float> conv_output3(128 * 8 * 8, 0.0f);
        for (int f = 0; f < 128; ++f) {
            for (int i = 0; i < 8; ++i) {
                for (int j = 0; j < 8; ++j) {
                    float sum = 0.0f;
                    for (int ki = 0; ki < 3; ++ki) {
                        for (int kj = 0; kj < 3; ++kj) {
                            sum += conv_output2[(i + ki) * 16 + (j + kj)] * conv3_weights[f][ki * 3 + kj];
                        }
                    }
                    conv_output3[f * 8 * 8 + i * 8 + j] = std::max(0.0f, sum);
                }
            }
        }

        std::vector<float> fc1_output(256, 0.0f);
        for (int i = 0; i < 256; ++i) {
            float sum = 0.0f;
            for (int j = 0; j < 128 * 8 * 8; ++j) {
                sum += conv_output3[j] * fc1_weights[j];
            }
            fc1_output[i] = std::max(0.0f, sum);
        }

        std::vector<float> fc2_output(128, 0.0f);
        for (int i = 0; i < 128; ++i) {
            float sum = 0.0f;
            for (int j = 0; j < 256; ++j) {
                sum += fc1_output[j] * fc2_weights[j];
            }
            fc2_output[i] = std::max(0.0f, sum);
        }

        std::vector<float> output(NUM_CLASSES, 0.0f);
        for (int i = 0; i < NUM_CLASSES; ++i) {
            float sum = 0.0f;
            for (int j = 0; j < 128; ++j) {
                sum += fc2_output[j] * fc3_weights[j];
            }
            output[i] = sum;
        }

        return output;
    }

    void backward(const float input[IMAGE_SIZE][IMAGE_SIZE], const std::vector<float>& output, int label, float learning_rate) {
        std::vector<float> output_grad(NUM_CLASSES, 0.0f);
        output_grad[label] = 1.0f - output[label];

        std::vector<float> fc3_grad(128, 0.0f);
        for (int i = 0; i < NUM_CLASSES; ++i) {
            for (int j = 0; j < 128; ++j) {
                fc3_grad[j] += output_grad[i];
                fc3_weights[j] += learning_rate * output_grad[i];
            }
        }

        std::vector<float> fc2_grad(256, 0.0f);
        for (int i = 0; i < 128; ++i) {
            for (int j = 0; j < 256; ++j) {
                fc2_grad[j] += fc3_grad[i];
                fc2_weights[j] += learning_rate * fc3_grad[i];
            }
        }

        std::vector<float> conv3_grad(128 * 8 * 8, 0.0f);
        for (int f = 0; f < 128; ++f) {
            for (int i = 0; i < 8; ++i) {
                for (int j = 0; j < 8; ++j) {
                    for (int ki = 0; ki < 3; ++ki) {
                        for (int kj = 0; kj < 3; ++kj) {
                            conv3_grad[f * 8 * 8 + i * 8 + j] += fc2_grad[f * 8 * 8 + i * 8 + j];
                            conv3_weights[f][ki * 3 + kj] += learning_rate * conv3_grad[f * 8 * 8 + i * 8 + j];
                        }
                    }
                }
            }
        }

        std::vector<float> conv2_grad(64 * 16 * 16, 0.0f);
        for (int f = 0; f < 64; ++f) {
            for (int i = 0; i < 16; ++i) {
                for (int j = 0; j < 16; ++j) {
                    for (int ki = 0; ki < 3; ++ki) {
                        for (int kj = 0; kj < 3; ++kj) {
                            conv2_grad[f * 16 * 16 + i * 16 + j] += conv3_grad[f * 16 * 16 + i * 16 + j];
                            conv2_weights[f][ki * 3 + kj] += learning_rate * conv2_grad[f * 16 * 16 + i * 16 + j];
                        }
                    }
                }
            }
        }

        std::vector<float> conv1_grad(32 * 32 * 32, 0.0f);
        for (int f = 0; f < 32; ++f) {
            for (int i = 0; i < 32; ++i) {
                for (int j = 0; j < 32; ++j) {
                    for (int ki = 0; ki < 3; ++ki) {
                        for (int kj = 0; kj < 3; ++kj) {
                            conv1_grad[f * 32 * 32 + i * 32 + j] += conv2_grad[f * 32 * 32 + i * 32 + j];
                            conv1_weights[f][ki * 3 + kj] += learning_rate * conv1_grad[f * 32 * 32 + i * 32 + j];
                        }
                    }
                }
            }
        }
    }

    void train_batch(const std::vector<Image>& batch, float learning_rate) {
        for (const auto& img : batch) {
            auto output = forward(img.data);
            backward(img.data, output, img.label, learning_rate);
        }
    }

    void augment_data(std::vector<Image>& batch) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        for (auto& img : batch) {
            if (dis(gen) > 0.5) {
                // Apply horizontal flip
                for (int i = 0; i < IMAGE_SIZE; ++i) {
                    std::reverse(img.data[i], img.data[i] + IMAGE_SIZE);
                }
            }
            if (dis(gen) > 0.5) {
                // Apply random rotation (90 degrees)
                float temp[IMAGE_SIZE][IMAGE_SIZE];
                for (int i = 0; i < IMAGE_SIZE; ++i) {
                    for (int j = 0; j < IMAGE_SIZE; ++j) {
                        temp[j][IMAGE_SIZE - 1 - i] = img.data[i][j];
                    }
                }
                std::copy(&temp[0][0], &temp[0][0] + IMAGE_SIZE * IMAGE_SIZE, &img.data[0][0]);
            }
        }
    }

    template <typename T>
    void save_vector(std::ofstream& file, const std::vector<T>& vec) {
        size_t size = vec.size();
        file.write(reinterpret_cast<const char*>(&size), sizeof(size));
        file.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(T));
    }

    template <typename T>
    void load_vector(std::ifstream& file, std::vector<T>& vec) {
        size_t size;
        file.read(reinterpret_cast<char*>(&size), sizeof(size));
        vec.resize(size);
        file.read(reinterpret_cast<char*>(vec.data()), size * sizeof(T));
    }
};

#endif // CNN_MODEL_H