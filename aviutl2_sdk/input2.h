//----------------------------------------------------------------------------------
//	���̓v���O�C�� �w�b�_�[�t�@�C�� for AviUtl ExEdit2
//	By �j�d�m����
//----------------------------------------------------------------------------------

// ���̓t�@�C�����\����
// �摜�t�H�[�}�b�g��RGB24bit,RGBA32bit,PA64,HF64,YUY2,YC48���Ή����Ă��܂�
// �����t�H�[�}�b�g��PCM16bit,PCM(float)32bit���Ή����Ă��܂�
// ��PA64��DXGI_FORMAT_R16G16B16A16_UNORM(��Z�ς݃�)�ł�
// ��HF64��DXGI_FORMAT_R16G16B16A16_FLOAT(��Z�ς݃�)�ł�
// ��YC48�͌݊��Ή��̋������t�H�[�}�b�g�ł� 
struct INPUT_INFO {
	int	flag;					// �t���O
	static constexpr int FLAG_VIDEO = 1; // �摜�f�[�^����
	static constexpr int FLAG_AUDIO = 2; // �����f�[�^����
	int	rate, scale;			// �t���[�����[�g�A�X�P�[��
	int	n;						// �t���[����
	BITMAPINFOHEADER* format;	// �摜�t�H�[�}�b�g�ւ̃|�C���^(���Ɋ֐����Ă΂��܂œ��e��L���ɂ��Ă���)
	int	format_size;			// �摜�t�H�[�}�b�g�̃T�C�Y
	int audio_n;				// �����T���v����
	WAVEFORMATEX* audio_format;	// �����t�H�[�}�b�g�ւ̃|�C���^(���Ɋ֐����Ă΂��܂œ��e��L���ɂ��Ă���)
	int audio_format_size;		// �����t�H�[�}�b�g�̃T�C�Y
};

// ���̓t�@�C���n���h��
typedef void* INPUT_HANDLE;

// ���̓v���O�C���\����
struct INPUT_PLUGIN_TABLE {
	int flag;					// �t���O
	static constexpr int FLAG_VIDEO = 1; //	�摜���T�|�[�g����
	static constexpr int FLAG_AUDIO = 2; //	�������T�|�[�g����
	static constexpr int FLAG_CONCURRENT = 16; // �摜�E�����f�[�^�̓����擾���T�|�[�g���� ���摜�Ɖ����擾�֐��������ɌĂ΂��
	LPCWSTR name;				// �v���O�C���̖��O
	LPCWSTR filefilter;			// ���̓t�@�C���t�B���^
	LPCWSTR information;		// �v���O�C���̏��

	// ���̓t�@�C�����I�[�v������֐��ւ̃|�C���^
	// file		: �t�@�C����
	// �߂�l	: TRUE�Ȃ���̓t�@�C���n���h��
	INPUT_HANDLE (*func_open)(LPCWSTR file);

	// ���̓t�@�C�����N���[�Y����֐��ւ̃|�C���^
	// ih		: ���̓t�@�C���n���h��
	// �߂�l	: TRUE�Ȃ琬��
	bool (*func_close)(INPUT_HANDLE ih);

	// ���̓t�@�C���̏����擾����֐��ւ̃|�C���^
	// ih		: ���̓t�@�C���n���h��
	// iip		: ���̓t�@�C�����\���̂ւ̃|�C���^
	// �߂�l	: TRUE�Ȃ琬��
	bool (*func_info_get)(INPUT_HANDLE ih, INPUT_INFO* iip);

	// �摜�f�[�^��ǂݍ��ފ֐��ւ̃|�C���^
	// ih		: ���̓t�@�C���n���h��
	// frame	: �ǂݍ��ރt���[���ԍ�
	// buf		: �f�[�^��ǂݍ��ރo�b�t�@�ւ̃|�C���^
	// �߂�l	: �ǂݍ��񂾃f�[�^�T�C�Y
	int (*func_read_video)(INPUT_HANDLE ih, int frame, void* buf);

	// �����f�[�^��ǂݍ��ފ֐��ւ̃|�C���^
	// ih		: ���̓t�@�C���n���h��
	// start	: �ǂݍ��݊J�n�T���v���ԍ�
	// length	: �ǂݍ��ރT���v����
	// buf		: �f�[�^��ǂݍ��ރo�b�t�@�ւ̃|�C���^
	// �߂�l	: �ǂݍ��񂾃T���v����
	int (*func_read_audio)(INPUT_HANDLE ih, int start, int length, void* buf);

	// ���͐ݒ�̃_�C�A���O��v�����ꂽ���ɌĂ΂��֐��ւ̃|�C���^ (nullptr�Ȃ�Ă΂�܂���)
	// hwnd			: �E�B���h�E�n���h��
	// dll_hinst	: �C���X�^���X�n���h��
	// �߂�l		: TRUE�Ȃ琬��
	bool (*func_config)(HWND hwnd, HINSTANCE dll_hinst);

	// �g���p�̗\��
	void (*reserve0)();
	void (*reserve1)();
};
