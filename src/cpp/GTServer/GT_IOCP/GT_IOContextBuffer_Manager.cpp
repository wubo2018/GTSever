#include "GT_IOContextBuffer_Manager.h"
#include "GTUtlity/GT_Util_CfgHelper.h"
#include "GTUtlity/GT_Util_GlogWrapper.h"

namespace GT {

    namespace NET {

#define IOBUFFER_MANAGER_LOCK_THIS_SCOPE	std::lock_guard<std::mutex> lk(GT_IOContextBuffer_Manager::io_buffer_manager_mutex_);
		std::mutex GT_IOContextBuffer_Manager::io_buffer_manager_mutex_;

        GT_IOContextBuffer_Manager::GT_IOContextBuffer_Manager():io_buffer_manager_inited_(false), 
												   io_buffer_size_(4*1024), 
			                                       pre_allocate_size_(2000), 
			                                       re_allocate_size_(1000),
													size_need_reallocate_(50)
        {
			io_buffer_cache_.clear();
			io_buffer_in_use_.clear();
        }


        GT_IOContextBuffer_Manager::~GT_IOContextBuffer_Manager()
        {
        }

		GT_IOContextBuffer_Manager& GT_IOContextBuffer_Manager::GetInstance() {
			IOBUFFER_MANAGER_LOCK_THIS_SCOPE;
			static GT_IOContextBuffer_Manager gt_io_buffer_manager_;
			return gt_io_buffer_manager_;
		}

		bool GT_IOContextBuffer_Manager::Initialize() {
			IOBUFFER_MANAGER_LOCK_THIS_SCOPE;
			if (io_buffer_manager_inited_) {
				GT_LOG_INFO("IO Buffer Manager already init!");
				return true;
			}

			GT_LOG_INFO("Now init IO Buffer Manager");
			bool ret = PreAllocateSomeIOBuffer_();
			if (!ret) {
				GT_LOG_ERROR("pre allocate io buffer failed");
			}
			else {
				io_buffer_manager_inited_ = true;
			}

			pre_allocate_size_	= GT_READ_CFG_INT("io_buffer_cfg", "pre_allocate_size", 2000);
			io_buffer_size_		= GT_READ_CFG_INT("io_buffer_cfg", "buffer_size", 4 * 1024);
			re_allocate_size_	= GT_READ_CFG_INT("io_buffer_cfg", "reallocate_size", 1000);

			return ret;
		}


		bool GT_IOContextBuffer_Manager::PreAllocateSomeIOBuffer_() {
			GT_LOG_INFO("pre allocate some io buffer size = "<< pre_allocate_size_ <<", each buffer size = " << io_buffer_size_ << " (Bytes)");

			for (size_t buffer_size = 0; buffer_size < pre_allocate_size_; buffer_size++) {
				IO_BUFFER_PTR temp_ptr = IO_BUFFER_PTR(new GT_IOContextBuffer(io_buffer_size_));
				if (temp_ptr->AllocateIOBuffer())
					io_buffer_cache_.push_back(temp_ptr);
				else
					continue;
			}
			return !io_buffer_cache_.empty();
		}

		bool GT_IOContextBuffer_Manager::ReAllocateSomeIOBuffer_() {
			GT_LOG_INFO("io buffer cache size is not enough, reallocate some io buffers, reallocate size = "<< re_allocate_size_);
			for (size_t s = 0; s < re_allocate_size_; s++) {
				IO_BUFFER_PTR temp_ptr = IO_BUFFER_PTR(new GT_IOContextBuffer(io_buffer_size_));
				if (temp_ptr->AllocateIOBuffer())
					io_buffer_cache_.push_back(temp_ptr);
				else
					continue;
			}
			return io_buffer_cache_.empty();
		}

		void GT_IOContextBuffer_Manager::CleanIOBufferCache_() {
			io_buffer_cache_.clear();
			io_buffer_in_use_.clear();
		}

		IO_BUFFER_PTR GT_IOContextBuffer_Manager::GetNextIOBufferPtr() {
			IOBUFFER_MANAGER_LOCK_THIS_SCOPE;
			IO_BUFFER_PTR temp_ptr;
			if (io_buffer_cache_.empty() || io_buffer_cache_.size() < size_need_reallocate_) {
				ReAllocateSomeIOBuffer_();
			}else{
				temp_ptr = io_buffer_cache_.front();
				io_buffer_cache_.pop_front();
				io_buffer_in_use_.push_back(temp_ptr);
				return io_buffer_in_use_.back();
			}
			return nullptr;
		}

		void GT_IOContextBuffer_Manager::ReleaseIOBuffer(IO_BUFFER_PTR buffer_ptr) {
			IOBUFFER_MANAGER_LOCK_THIS_SCOPE;
			if (buffer_ptr != nullptr) {
				buffer_ptr->ResetBuffer();
			}
			io_buffer_cache_.push_back(buffer_ptr);
		}

		void GT_IOContextBuffer_Manager::Finalize() {
			IOBUFFER_MANAGER_LOCK_THIS_SCOPE;
			CleanIOBufferCache_();
			io_buffer_manager_inited_ = false;
		}
    }
}
