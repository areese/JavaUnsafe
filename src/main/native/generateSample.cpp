#include <sys/param.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "generateSample.h"
/**
 * This function was auto-generated
 * Given an allocated long addres, len tuple
 * It will encode in a way compatible with the generated java.
 * everything is 64bit longs with a cast
 * Strings are considered UTF8, and are a tuple of address + length
 * Due to native memory tracking, strings are prealloacted with Unsafe.allocateMemory and assigned an output length
 * Similiar to how a c function would take char *outBuf, size_t bufLen
 * The length coming in says how large the buffer for address is.
 * The length coming out says how many characters including \0 were written
**/
void encodeIntoJava_SampleInfo(SampleInfoStruct *inputData, long address, long addressLength) {
    uint64_t offset = 0;
    {
        uint64_t *typePtr = (uint64_t*)(address + offset); // int
        offset += 8;
        (*typePtr) = inputData->type;
    }
    {
        uint64_t *attrsPtr = (uint64_t*)(address + offset); // int
        offset += 8;
        (*attrsPtr) = inputData->attrs;
    }
    {
        uint64_t *statusPtr = (uint64_t*)(address + offset); // int
        offset += 8;
        (*statusPtr) = inputData->status;
    }
    {
        uint64_t *expirationPtr = (uint64_t*)(address + offset); // long
        offset += 8;
        (*expirationPtr) = inputData->expiration;
    }
    {
        uint64_t *readCountPtr = (uint64_t*)(address + offset); // int
        offset += 8;
        (*readCountPtr) = inputData->readCount;
    }
    {
        uint64_t *writeCountPtr = (uint64_t*)(address + offset); // int
        offset += 8;
        (*writeCountPtr) = inputData->writeCount;
    }
    {
        uint64_t *writeTimestampPtr = (uint64_t*)(address + offset); // long
        offset += 8;
        (*writeTimestampPtr) = inputData->writeTimestamp;
    }
    {
        uint64_t *iaPtr = *(uint64_t**)(address + offset); // java.net.InetAddress
        offset += 8;

        uint64_t *iaLenPtr = (uint64_t*)(address + offset); // java.net.InetAddress
        offset += 8;


    }

    {
        {
            uint64_t *ptr = (uint64_t *)address;
            fprintf(stderr,"Dumping address at at start 0x%lx len 0x%lx\n", address, addressLength);
            for (long l=0; l<addressLength/sizeof(uint64_t); l++) {
                fprintf(stderr,"%llx\n",ptr[l]);
            }
            fprintf(stderr,"\n\n");
        }
        uint64_t *orgPtr = *(uint64_t**)(address + offset); // java.lang.String
        offset += 8;

        uint64_t *orgLenPtr = (uint64_t*)(address + offset); // java.lang.String
        offset += 8;

        // use the shortest of buffersize and input size
        (*orgLenPtr) = MIN( (*orgLenPtr), inputData->org.len);

        {
            uint64_t *ptr = (uint64_t *)address;
            fprintf(stderr,"Dumping address at Before copy of org.address 0x%lx len 0x%lx\n", address, addressLength);
            for (long l=0; l<addressLength/sizeof(uint64_t); l++) {
                fprintf(stderr,"%llx\n",ptr[l]);
            }
            fprintf(stderr,"\n\n");
        }
        fprintf(stderr, "encoding to 0x%llx of len 0x%llx\n", inputData->org.address, (*orgLenPtr));
        memcpy ((void*) orgPtr, (void*) inputData->org.address, (*orgLenPtr));
        {
            uint64_t *ptr = (uint64_t *)address;
            fprintf(stderr,"Dumping address at After copy of org.address 0x%lx len 0x%lx\n", address, addressLength);
            for (long l=0; l<addressLength/sizeof(uint64_t); l++) {
                fprintf(stderr,"%llx\n",ptr[l]);
            }
            fprintf(stderr,"\n\n");
        }
    }

    {
        {
            uint64_t *ptr = (uint64_t *)address;
            fprintf(stderr,"Dumping address at at start 0x%lx len 0x%lx\n", address, addressLength);
            for (long l=0; l<addressLength/sizeof(uint64_t); l++) {
                fprintf(stderr,"%llx\n",ptr[l]);
            }
            fprintf(stderr,"\n\n");
        }
        uint64_t *locPtr = *(uint64_t**)(address + offset); // java.lang.String
        offset += 8;

        uint64_t *locLenPtr = (uint64_t*)(address + offset); // java.lang.String
        offset += 8;

        // use the shortest of buffersize and input size
        (*locLenPtr) = MIN( (*locLenPtr), inputData->loc.len);

        {
            uint64_t *ptr = (uint64_t *)address;
            fprintf(stderr,"Dumping address at Before copy of loc.address 0x%lx len 0x%lx\n", address, addressLength);
            for (long l=0; l<addressLength/sizeof(uint64_t); l++) {
                fprintf(stderr,"%llx\n",ptr[l]);
            }
            fprintf(stderr,"\n\n");
        }
        fprintf(stderr, "encoding to 0x%llx of len 0x%llx\n", inputData->loc.address, (*locLenPtr));
        memcpy ((void*) locPtr, (void*) inputData->loc.address, (*locLenPtr));
        {
            uint64_t *ptr = (uint64_t *)address;
            fprintf(stderr,"Dumping address at After copy of loc.address 0x%lx len 0x%lx\n", address, addressLength);
            for (long l=0; l<addressLength/sizeof(uint64_t); l++) {
                fprintf(stderr,"%llx\n",ptr[l]);
            }
            fprintf(stderr,"\n\n");
        }
    }

    {
        {
            uint64_t *ptr = (uint64_t *)address;
            fprintf(stderr,"Dumping address at at start 0x%lx len 0x%lx\n", address, addressLength);
            for (long l=0; l<addressLength/sizeof(uint64_t); l++) {
                fprintf(stderr,"%llx\n",ptr[l]);
            }
            fprintf(stderr,"\n\n");
        }
        uint64_t *ccodePtr = *(uint64_t**)(address + offset); // java.lang.String
        offset += 8;

        uint64_t *ccodeLenPtr = (uint64_t*)(address + offset); // java.lang.String
        offset += 8;

        // use the shortest of buffersize and input size
        (*ccodeLenPtr) = MIN( (*ccodeLenPtr), inputData->ccode.len);

        {
            uint64_t *ptr = (uint64_t *)address;
            fprintf(stderr,"Dumping address at Before copy of ccode.address 0x%lx len 0x%lx\n", address, addressLength);
            for (long l=0; l<addressLength/sizeof(uint64_t); l++) {
                fprintf(stderr,"%llx\n",ptr[l]);
            }
            fprintf(stderr,"\n\n");
        }
        fprintf(stderr, "encoding to 0x%llx of len 0x%llx\n", inputData->ccode.address, (*ccodeLenPtr));
        memcpy ((void*) ccodePtr, (void*) inputData->ccode.address, (*ccodeLenPtr));
        {
            uint64_t *ptr = (uint64_t *)address;
            fprintf(stderr,"Dumping address at After copy of ccode.address 0x%lx len 0x%lx\n", address, addressLength);
            for (long l=0; l<addressLength/sizeof(uint64_t); l++) {
                fprintf(stderr,"%llx\n",ptr[l]);
            }
            fprintf(stderr,"\n\n");
        }
    }

    {
        {
            uint64_t *ptr = (uint64_t *)address;
            fprintf(stderr,"Dumping address at at start 0x%lx len 0x%lx\n", address, addressLength);
            for (long l=0; l<addressLength/sizeof(uint64_t); l++) {
                fprintf(stderr,"%llx\n",ptr[l]);
            }
            fprintf(stderr,"\n\n");
        }
        uint64_t *descPtr = *(uint64_t**)(address + offset); // java.lang.String
        offset += 8;

        uint64_t *descLenPtr = (uint64_t*)(address + offset); // java.lang.String
        offset += 8;

        // use the shortest of buffersize and input size
        (*descLenPtr) = MIN( (*descLenPtr), inputData->desc.len);

        {
            uint64_t *ptr = (uint64_t *)address;
            fprintf(stderr,"Dumping address at Before copy of desc.address 0x%lx len 0x%lx\n", address, addressLength);
            for (long l=0; l<addressLength/sizeof(uint64_t); l++) {
                fprintf(stderr,"%llx\n",ptr[l]);
            }
            fprintf(stderr,"\n\n");
        }
        fprintf(stderr, "encoding to 0x%llx of len 0x%llx\n", inputData->desc.address, (*descLenPtr));
        memcpy ((void*) descPtr, (void*) inputData->desc.address, (*descLenPtr));
        {
            uint64_t *ptr = (uint64_t *)address;
            fprintf(stderr,"Dumping address at After copy of desc.address 0x%lx len 0x%lx\n", address, addressLength);
            for (long l=0; l<addressLength/sizeof(uint64_t); l++) {
                fprintf(stderr,"%llx\n",ptr[l]);
            }
            fprintf(stderr,"\n\n");
        }
    }

}

