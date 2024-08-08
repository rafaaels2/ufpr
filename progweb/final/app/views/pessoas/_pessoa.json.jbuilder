json.extract! pessoa, :id, :rg, :nome, :idade, :cidade, :created_at, :updated_at
json.url pessoa_url(pessoa, format: :json)
