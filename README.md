# JPEG Compression Project

A C++ implementation of the JPEG compression and decompression pipeline using **OpenCV**. This project demonstrates the fundamental steps of the JPEG standard, from color space transformation to entropy coding.

## Features

- **Full Compression Pipeline:** Implements the standard JPEG workflow (DCT, Quantization, Zig-Zag, RLE).
- **Custom DCT Implementation:** Manual computation of Forward and Inverse Discrete Cosine Transforms on 8x8 blocks.
- **Entropy Coding:** Includes Zig-Zag scanning and Run-Length Encoding (RLE) to a text-based format.
- **Performance Metrics:** Calculates compression rate and size reduction percentage.
- **Visual Feedback:** Displays original, compressed (quantized domain), and reconstructed images.

## Implementation Details

The project follows these sequential steps:

1.  **Color Space Conversion:** Transforms the image from BGR to `YCrCb`.
2.  **Padding:** Ensures image dimensions are multiples of 8 for block processing.
3.  **Level Shifting:** Subtracts 128 from pixel values to center them around zero.
4.  **FDCT (Forward DCT):** Converts 8x8 spatial blocks into the frequency domain.
5.  **Quantization:** Applies a standard Luminance Quantization Matrix (Q50) to discard high-frequency information.
6.  **Serialization:**
    *   **Zig-Zag Ordering:** Reorders 2D coefficients into a 1D sequence.
    *   **RLE (Run-Length Encoding):** Compresses the sequence by grouping identical consecutive values.
7.  **Decompression:** Reverses the above steps (RLE Decoding -> Dequantization -> IDCT -> Level Shifting -> BGR Conversion).

## Project Structure

- `OpenCVApplication.cpp`: Core logic and UI menu.
- `common.h/cpp`: Utility functions for file handling and image resizing.
- `Images/`: Sample images for testing.
- `rle.txt`: Intermediate file storing the encoded image data.

## Getting Started

### Prerequisites
- Visual Studio (Solution file provided)
- OpenCV 4.x

### Running the Application
1. Open `OpenCVApplication.sln` in Visual Studio.
2. Build the project in `x64` mode.
3. Run the application.
4. Select **Option 1** from the menu to test the compression on an image of your choice.

## Sample Output
Upon processing an image, the console will display:
- **Compression Rate:** Ratio between original and compressed non-zero coefficients.
- **Size Reduction:** Percentage of data reduced.