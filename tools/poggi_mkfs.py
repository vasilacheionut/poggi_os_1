#!/usr/bin/env python3
"""
PoggiFS Builder - Scrie fisiere pe imaginea de disc
Folosire: poggi_mkfs.py --image os_image.bin --root fs_root/
"""

import os
import sys
import struct
import argparse

SECTOR_SIZE = 512
INDEX_SECTOR = 55
DATA_START_SECTOR = 56

def write_sector(image, sector, data):
    """Scrie date aliniate la 512 octeti intr-un sector"""
    image.seek(sector * SECTOR_SIZE)
    # Asigura ca data are exact SECTOR_SIZE (completa cu zero)
    padded = data.ljust(SECTOR_SIZE, b'\x00')
    image.write(padded[:SECTOR_SIZE])

def create_index_entry(filename, lba, size):
    """Creeaza o intrare de 32 octeti: nume(16) + lba(4) + size(4) + padding(8)"""
    name_bytes = filename.encode('ascii')[:16].ljust(16, b'\x00')
    lba_bytes = struct.pack('<I', lba)
    size_bytes = struct.pack('<I', size)
    padding = b'\x00' * 8
    return name_bytes + lba_bytes + size_bytes + padding

def build_fs(image_path, root_dir):
    """Construieste sistemul de fisiere pe imagine"""
    # Deschide imaginea in mod citire+scriere
    with open(image_path, 'r+b') as img:
        # Colecteaza toate fisierele din root_dir
        entries = []
        current_lba = DATA_START_SECTOR
        
        for filename in sorted(os.listdir(root_dir)):
            filepath = os.path.join(root_dir, filename)
            if not os.path.isfile(filepath):
                continue
            # Citeste fisierul
            with open(filepath, 'rb') as f:
                data = f.read()
            # Calculeaza cate sectoare ocupa
            size = len(data)
            # Scrie datele in imagine
            img.seek(current_lba * SECTOR_SIZE)
            img.write(data)
            # Completeaza ultimul sector cu zero daca nu e multiplu
            remainder = size % SECTOR_SIZE
            if remainder:
                img.write(b'\x00' * (SECTOR_SIZE - remainder))
            
            # Adauga intrare in index
            entries.append(create_index_entry(filename, current_lba, size))
            # Avanseaza LBA
            current_lba += (size + SECTOR_SIZE - 1) // SECTOR_SIZE
        
        # Construieste sectorul index (primii 32*b entries, restul zero)
        index_sector_data = b''.join(entries)
        # Maxim 512/32 = 16 fisiere
        if len(entries) > 16:
            print("Warning: prea multe fisiere, doar primele 16 vor fi in index")
        # Scrie indexul in sectorul 55
        write_sector(img, INDEX_SECTOR, index_sector_data)

def main():
    parser = argparse.ArgumentParser(description='PoggiFS builder')
    parser.add_argument('--image', required=True, help='calea catre imaginea disc')
    parser.add_argument('--root', required=True, help='directorul cu fisierele sursa pentru PoggiFS')
    args = parser.parse_args()
    
    if not os.path.exists(args.root):
        print(f"Error: root directory {args.root} does not exist")
        sys.exit(1)
    
    build_fs(args.image, args.root)
    print(f"PoggiFS built successfully on {args.image} from {args.root}")

if __name__ == '__main__':
    main()