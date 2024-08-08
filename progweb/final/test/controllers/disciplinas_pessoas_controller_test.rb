require "test_helper"

class DisciplinasPessoasControllerTest < ActionDispatch::IntegrationTest
  setup do
    @disciplinas_pessoa = disciplinas_pessoas(:one)
  end

  test "should get index" do
    get disciplinas_pessoas_url
    assert_response :success
  end

  test "should get new" do
    get new_disciplinas_pessoa_url
    assert_response :success
  end

  test "should create disciplinas_pessoa" do
    assert_difference("DisciplinasPessoa.count") do
      post disciplinas_pessoas_url, params: { disciplinas_pessoa: { disciplina_id: @disciplinas_pessoa.disciplina_id, pessoa_id: @disciplinas_pessoa.pessoa_id } }
    end

    assert_redirected_to disciplinas_pessoa_url(DisciplinasPessoa.last)
  end

  test "should show disciplinas_pessoa" do
    get disciplinas_pessoa_url(@disciplinas_pessoa)
    assert_response :success
  end

  test "should get edit" do
    get edit_disciplinas_pessoa_url(@disciplinas_pessoa)
    assert_response :success
  end

  test "should update disciplinas_pessoa" do
    patch disciplinas_pessoa_url(@disciplinas_pessoa), params: { disciplinas_pessoa: { disciplina_id: @disciplinas_pessoa.disciplina_id, pessoa_id: @disciplinas_pessoa.pessoa_id } }
    assert_redirected_to disciplinas_pessoa_url(@disciplinas_pessoa)
  end

  test "should destroy disciplinas_pessoa" do
    assert_difference("DisciplinasPessoa.count", -1) do
      delete disciplinas_pessoa_url(@disciplinas_pessoa)
    end

    assert_redirected_to disciplinas_pessoas_url
  end
end
